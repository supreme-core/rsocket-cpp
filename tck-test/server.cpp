// Copyright 2004-present Facebook. All Rights Reserved.

#include <fstream>

#include <folly/Memory.h>
#include <folly/io/async/AsyncServerSocket.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include <gmock/gmock.h>

#include "src/NullRequestHandler.h"
#include "src/StandardReactiveSocket.h"
#include "src/SubscriptionBase.h"
#include "src/framed/FramedDuplexConnection.h"
#include "src/tcp/TcpDuplexConnection.h"

#include "test/simple/StatsPrinter.h"

#include "tck-test/MarbleProcessor.h"

using namespace ::testing;
using namespace ::reactivesocket;
using namespace ::folly;

DEFINE_string(address, "9898", "port to listen to");
DEFINE_string(test_file, "../tck-test/servertest.txt", "test file to run");

namespace {
class ServerSubscription : public SubscriptionBase {
 public:
  explicit ServerSubscription(
      std::shared_ptr<Subscriber<Payload>> response,
      std::shared_ptr<tck::MarbleProcessor> marbleProcessor,
      size_t /* numElems = 2 */)
      : ExecutorBase(defaultExecutor()),
        response_(std::move(response)),
        marbleProcessor_(std::move(marbleProcessor)) {}

 private:
  void requestImpl(size_t n) noexcept override {
    marbleProcessor_->request(n);
  }

  void cancelImpl() noexcept override {
    marbleProcessor_->cancel();
  }

  std::shared_ptr<Subscriber<Payload>> response_;
  std::shared_ptr<tck::MarbleProcessor> marbleProcessor_;
};

class Callback : public AsyncServerSocket::AcceptCallback {
 public:
  Callback(
      EventBase& eventBase,
      Stats& stats,
      std::map<std::pair<std::string, std::string>, std::string> reqRespMarbles,
      std::map<std::pair<std::string, std::string>, std::string> streamMarbles)
      : eventBase_(eventBase),
        stats_(stats),
        reqRespMarbles_(std::move(reqRespMarbles)),
        streamMarbles_(std::move(streamMarbles)){};

  virtual ~Callback() = default;

  virtual void connectionAccepted(
      int fd,
      const SocketAddress& clientAddr) noexcept override {
    LOG(INFO) << "ConnectionAccepted " << clientAddr.describe() << std::endl;

    auto socket =
        folly::AsyncSocket::UniquePtr(new AsyncSocket(&eventBase_, fd));

    std::unique_ptr<DuplexConnection> connection =
        std::make_unique<TcpDuplexConnection>(
            std::move(socket), inlineExecutor(), stats_);
    std::unique_ptr<DuplexConnection> framedConnection =
        std::make_unique<FramedDuplexConnection>(
            std::move(connection), inlineExecutor());
    std::unique_ptr<RequestHandler> requestHandler =
        std::make_unique<ServerRequestHandler>(*this);

    auto rs = StandardReactiveSocket::fromServerConnection(
        eventBase_,
        std::move(framedConnection),
        std::move(requestHandler),
        stats_);

    rs->onClosed([ this, rs = rs.get() ](const folly::exception_wrapper& ex) {
      removeSocket(*rs);
    });

    reactiveSockets_.push_back(std::move(rs));
  }

  void removeSocket(ReactiveSocket& socket) {
    if (!shuttingDown_) {
      reactiveSockets_.erase(std::remove_if(
          reactiveSockets_.begin(),
          reactiveSockets_.end(),
          [&socket](std::unique_ptr<StandardReactiveSocket>& vecSocket) {
            return vecSocket.get() == &socket;
          }));
    }
  }

  virtual void acceptError(const std::exception& ex) noexcept override {
    std::cout << "AcceptError " << ex.what() << std::endl;
  }

  void shutdown() {
    shuttingDown_ = true;
    reactiveSockets_.clear();
  }

 private:
  class ServerRequestHandler : public DefaultRequestHandler {
   public:
    explicit ServerRequestHandler(Callback& callback) : callback_(&callback) {}

    void handleRequestStream(
        Payload request,
        StreamId streamId,
        const std::shared_ptr<Subscriber<Payload>>&
            response) noexcept override {
      LOG(INFO) << "handleRequestStream " << request;
      std::string data = request.data->moveToFbString().toStdString();
      std::string metadata = request.metadata->moveToFbString().toStdString();
      auto it = callback_->streamMarbles_.find(std::make_pair(data, metadata));
      if (it == callback_->streamMarbles_.end()) {
        LOG(ERROR) << "No Handler found for the [data: " << data
                   << ", metadata:" << metadata << "]";
      } else {
        auto marbleProcessor =
            std::make_shared<tck::MarbleProcessor>(it->second, response);
        auto subscription =
            std::make_shared<ServerSubscription>(response, marbleProcessor, 0);
        response->onSubscribe(subscription);
        std::thread mpThread([marbleProcessor] { marbleProcessor->run(); });
        mpThread.detach();
      }
    }

    void handleRequestResponse(
        Payload request,
        StreamId streamId,
        const std::shared_ptr<Subscriber<Payload>>&
            response) noexcept override {
      LOG(INFO) << "handleRequestResponse " << request;
      std::string data = request.data->moveToFbString().toStdString();
      std::string metadata = request.metadata->moveToFbString().toStdString();
      auto it = callback_->reqRespMarbles_.find(std::make_pair(data, metadata));
      if (it == callback_->reqRespMarbles_.end()) {
        LOG(ERROR) << "No Handler found for the [data: " << data
                   << ", metadata:" << metadata << "]";
      } else {
        auto marbleProcessor =
            std::make_shared<tck::MarbleProcessor>(it->second, response);
        auto subscription =
            std::make_shared<ServerSubscription>(response, marbleProcessor, 0);
        response->onSubscribe(subscription);
        std::thread mpThread([marbleProcessor] { marbleProcessor->run(); });
        mpThread.detach();
      }
    }

    void handleFireAndForgetRequest(
        Payload request,
        StreamId streamId) noexcept override {
      LOG(INFO) << "handleFireAndForgetRequest " << request.moveDataToString();
    }

    void handleMetadataPush(
        std::unique_ptr<folly::IOBuf> request) noexcept override {
      LOG(INFO) << "handleMetadataPush " << request->moveToFbString();
    }

    std::shared_ptr<StreamState> handleSetupPayload(
        ReactiveSocket&,
        ConnectionSetupPayload request) noexcept override {
      LOG(INFO) << "handleSetupPayload " << request;
      return nullptr;
    }

   private:
    Callback* callback_;
  };

  std::vector<std::unique_ptr<StandardReactiveSocket>> reactiveSockets_;
  EventBase& eventBase_;
  Stats& stats_;
  bool shuttingDown_{false};
  std::map<std::pair<std::string, std::string>, std::string> reqRespMarbles_;
  std::map<std::pair<std::string, std::string>, std::string> streamMarbles_;
};
}

void signal_handler(int signal) {
  LOG(INFO) << "Terminating program after receiving signal " << signal;
  exit(signal);
}

std::tuple<
    std::map<
        std::pair<std::string, std::string> /*payload*/,
        std::string /*marble*/> /* ReqResp */,
    std::map<
        std::pair<std::string, std::string> /*payload*/,
        std::string /*marble*/> /* Stream */,
    std::map<
        std::pair<std::string, std::string> /*payload*/,
        std::string /*marble*/> /* Channel */>
parseMarbles(const std::string& fileName) {
  std::map<std::pair<std::string, std::string>, std::string> reqRespMarbles;
  std::map<std::pair<std::string, std::string>, std::string> streamMarbles;
  std::map<std::pair<std::string, std::string>, std::string> channelMarbles;

  std::ifstream input(fileName);
  if (!input.good()) {
    LOG(FATAL) << "Could not read from file '" << fileName << "'";
  }

  std::string line;
  while (std::getline(input, line)) {
    std::vector<folly::StringPiece> args;
    folly::split("%%", line, args);
    CHECK(args.size() == 4);
    if (args[0] == "rr") {
      reqRespMarbles.emplace(
          std::make_pair(args[1].toString(), args[2].toString()),
          args[3].toString());
    } else if (args[0] == "rs") {
      streamMarbles.emplace(
          std::make_pair(args[1].toString(), args[2].toString()),
          args[3].toString());
    } else if (args[0] == "channel") {
      channelMarbles.emplace(
          std::make_pair(args[1].toString(), args[2].toString()),
          args[3].toString());
    } else {
      LOG(FATAL) << "Unrecognized token " << args[0];
    }
  }
  return std::tie(reqRespMarbles, streamMarbles, channelMarbles);
}

int main(int argc, char* argv[]) {
  FLAGS_logtostderr = true;

  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();

  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  folly::ScopedEventBaseThread evbt;
  reactivesocket::StatsPrinter statsPrinter;

  auto marbles = parseMarbles(FLAGS_test_file);
  Callback callback(
      *evbt.getEventBase(),
      statsPrinter,
      std::get<0>(marbles), /* reqRespMarbles */
      std::get<1>(marbles)); /* streamMarbles */

  auto serverSocket = AsyncServerSocket::newSocket(evbt.getEventBase());

  evbt.getEventBase()->runInEventBaseThreadAndWait(
      [&evbt, &serverSocket, &callback]() {
        folly::SocketAddress addr;
        addr.setFromLocalIpPort(FLAGS_address);
        serverSocket->setReusePortEnabled(true);
        serverSocket->bind(addr);
        serverSocket->addAcceptCallback(&callback, evbt.getEventBase());
        serverSocket->listen(10);
        serverSocket->startAccepting();
        LOG(INFO) << "Server listening on " << addr.describe();
      });

  while (true)
    ;
}