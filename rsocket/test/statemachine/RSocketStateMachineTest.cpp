// Copyright 2004-present Facebook. All Rights Reserved.

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <yarpl/single/SingleSubscriptions.h>
#include <yarpl/single/Singles.h>
#include <yarpl/test_utils/Mocks.h>

#include "rsocket/RSocketResponder.h"
#include "rsocket/framing/FrameTransportImpl.h"
#include "rsocket/internal/Common.h"
#include "rsocket/statemachine/ChannelRequester.h"
#include "rsocket/statemachine/ChannelResponder.h"
#include "rsocket/statemachine/RequestResponseResponder.h"
#include "rsocket/test/test_utils/MockDuplexConnection.h"
#include "rsocket/test/test_utils/MockStreamsWriter.h"

using namespace testing;
using namespace yarpl::mocks;
using namespace yarpl::single;
using namespace yarpl::flowable;

namespace rsocket {

class ResponderMock : public RSocketResponder {
 public:
  MOCK_METHOD2(
      handleRequestResponse,
      std::shared_ptr<Single<Payload>>(Payload, StreamId));
  MOCK_METHOD2(
      handleRequestStream,
      std::shared_ptr<yarpl::flowable::Flowable<Payload>>(Payload, StreamId));
  MOCK_METHOD3(
      handleRequestChannel,
      std::shared_ptr<yarpl::flowable::Flowable<Payload>>(
          Payload request,
          std::shared_ptr<yarpl::flowable::Flowable<Payload>> requestStream,
          StreamId streamId));
};

class RSocketStateMachineTest : public Test {
 public:
  auto createClient(
      std::unique_ptr<MockDuplexConnection> connection,
      std::shared_ptr<RSocketResponder> responder) {
    EXPECT_CALL(*connection, setInput_(_));
    EXPECT_CALL(*connection, isFramed());

    auto transport =
        std::make_shared<FrameTransportImpl>(std::move(connection));

    auto stateMachine = std::make_shared<RSocketStateMachine>(
        std::move(responder),
        nullptr,
        RSocketMode::CLIENT,
        nullptr,
        nullptr,
        nullptr,
        nullptr);

    SetupParameters setupParameters;
    setupParameters.resumable = false; // Not resumable!
    stateMachine->connectClient(
        std::move(transport), std::move(setupParameters));

    return stateMachine;
  }

  const std::unordered_map<StreamId, StreamStateElem>& getStreams(
      RSocketStateMachine& stateMachine) {
    return stateMachine.streams_;
  }

  void setupRequestStream(
      RSocketStateMachine& stateMachine,
      StreamId streamId,
      uint32_t requestN,
      Payload payload) {
    stateMachine.setupRequestStream(streamId, requestN, std::move(payload));
  }

  void setupRequestChannel(
      RSocketStateMachine& stateMachine,
      StreamId streamId,
      uint32_t requestN,
      Payload payload) {
    stateMachine.setupRequestChannel(streamId, requestN, std::move(payload));
  }

  void setupRequestResponse(
      RSocketStateMachine& stateMachine,
      StreamId streamId,
      Payload payload) {
    stateMachine.setupRequestResponse(streamId, std::move(payload));
  }

  void setupFireAndForget(
      RSocketStateMachine& stateMachine,
      StreamId streamId,
      Payload payload) {
    stateMachine.setupFireAndForget(streamId, std::move(payload));
  }
};

TEST_F(RSocketStateMachineTest, RequestStream) {
  auto connection = std::make_unique<StrictMock<MockDuplexConnection>>();
  // Setup frame and request stream frame
  EXPECT_CALL(*connection, send_(_)).Times(2);

  auto stateMachine =
      createClient(std::move(connection), std::make_shared<RSocketResponder>());

  auto subscriber = std::make_shared<StrictMock<MockSubscriber<Payload>>>();
  EXPECT_CALL(*subscriber, onSubscribe_(_));
  EXPECT_CALL(*subscriber, onComplete_());

  stateMachine->requestStream(Payload{}, subscriber);

  auto& streams = getStreams(*stateMachine);
  ASSERT_EQ(1, streams.size());

  // This line causes: subscriber.onComplete()
  streams.at(1).stateMachine->endStream(StreamCompletionSignal::CANCEL);

  stateMachine->close({}, StreamCompletionSignal::CONNECTION_END);
}

TEST_F(RSocketStateMachineTest, RequestChannel) {
  auto connection = std::make_unique<StrictMock<MockDuplexConnection>>();
  // Setup frame and request channel frame
  EXPECT_CALL(*connection, send_(_)).Times(2);

  auto stateMachine =
      createClient(std::move(connection), std::make_shared<RSocketResponder>());

  auto in = std::make_shared<StrictMock<MockSubscriber<Payload>>>();
  EXPECT_CALL(*in, onSubscribe_(_));
  EXPECT_CALL(*in, onComplete_());

  auto out = stateMachine->requestChannel(Payload{}, true, in);
  auto subscription = std::make_shared<StrictMock<MockSubscription>>();
  EXPECT_CALL(*subscription, cancel_());
  out->onSubscribe(subscription);

  auto& streams = getStreams(*stateMachine);
  ASSERT_EQ(1, streams.size());

  // This line causes: in.onComplete() and outSubscription.cancel()
  streams.at(1).stateMachine->endStream(StreamCompletionSignal::CANCEL);

  stateMachine->close({}, StreamCompletionSignal::CONNECTION_END);
}

TEST_F(RSocketStateMachineTest, RequestResponse) {
  auto connection = std::make_unique<StrictMock<MockDuplexConnection>>();
  // Setup frame and request channel frame
  EXPECT_CALL(*connection, send_(_)).Times(2);

  auto stateMachine =
      createClient(std::move(connection), std::make_shared<RSocketResponder>());

  auto in = std::make_shared<SingleObserverBase<Payload>>();
  stateMachine->requestResponse(Payload{}, in);

  auto& streams = getStreams(*stateMachine);
  ASSERT_EQ(1, streams.size());

  // This line closes the stream
  streams.at(1).stateMachine->handlePayload(
      Payload{"test", "123"}, true, false);

  stateMachine->close({}, StreamCompletionSignal::CONNECTION_END);
}

TEST_F(RSocketStateMachineTest, RespondStream) {
  auto connection = std::make_unique<StrictMock<MockDuplexConnection>>();
  int requestCount = 5;
  // + the cancel frame when the stateMachine gets destroyed
  EXPECT_CALL(*connection, send_(_)).Times(requestCount + 1);

  int sendCount = 0;
  auto responder = std::make_shared<StrictMock<ResponderMock>>();
  EXPECT_CALL(*responder, handleRequestStream(_, _))
      .WillOnce(Return(
          yarpl::flowable::Flowable<Payload>::fromGenerator([&sendCount]() {
            ++sendCount;
            return Payload{};
          })));

  auto stateMachine = createClient(std::move(connection), responder);
  setupRequestStream(*stateMachine, 2, requestCount, Payload{});
  EXPECT_EQ(requestCount, sendCount);

  auto& streams = getStreams(*stateMachine);
  EXPECT_EQ(1, streams.size());

  // releases connection and the responder
  stateMachine->close({}, StreamCompletionSignal::CONNECTION_END);
}

TEST_F(RSocketStateMachineTest, RespondChannel) {
  auto connection = std::make_unique<StrictMock<MockDuplexConnection>>();
  int requestCount = 5;
  // + the cancel frame when the stateMachine gets destroyed
  EXPECT_CALL(*connection, send_(_)).Times(requestCount + 1);

  int sendCount = 0;
  auto responder = std::make_shared<StrictMock<ResponderMock>>();
  EXPECT_CALL(*responder, handleRequestChannel(_, _, _))
      .WillOnce(Return(
          yarpl::flowable::Flowable<Payload>::fromGenerator([&sendCount]() {
            ++sendCount;
            return Payload{};
          })));

  auto stateMachine = createClient(std::move(connection), responder);
  setupRequestChannel(*stateMachine, 2, requestCount, Payload{});
  EXPECT_EQ(requestCount, sendCount);

  auto& streams = getStreams(*stateMachine);
  EXPECT_EQ(1, streams.size());

  // releases connection and the responder
  stateMachine->close({}, StreamCompletionSignal::CONNECTION_END);
}

TEST_F(RSocketStateMachineTest, RespondRequest) {
  auto connection = std::make_unique<StrictMock<MockDuplexConnection>>();
  EXPECT_CALL(*connection, send_(_)).Times(2);

  int sendCount = 0;
  auto responder = std::make_shared<StrictMock<ResponderMock>>();
  EXPECT_CALL(*responder, handleRequestResponse(_, _))
      .WillOnce(Return(Singles::fromGenerator<Payload>([&sendCount]() {
        ++sendCount;
        return Payload{};
      })));

  auto stateMachine = createClient(std::move(connection), responder);
  setupRequestResponse(*stateMachine, 2, Payload{});
  EXPECT_EQ(sendCount, 1);

  auto& streams = getStreams(*stateMachine);
  EXPECT_EQ(0, streams.size()); // already completed

  // releases connection and the responder
  stateMachine->close({}, StreamCompletionSignal::CONNECTION_END);
}

} // namespace rsocket