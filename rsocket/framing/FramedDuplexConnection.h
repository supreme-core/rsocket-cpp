// Copyright (c) Facebook, Inc. and its affiliates.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

//#pragma once

#ifndef FramedDuplexConnection_H
#define FramedDuplexConnection_H
#endif


#include "rsocket/DuplexConnection.h"
#include "rsocket/internal/Common.h"
#include <scapix/bridge/object.h>


namespace rsocket {

class FramedReader;
struct ProtocolVersion;

class FramedDuplexConnection : public virtual DuplexConnection {
 public:
  FramedDuplexConnection(
      std::unique_ptr<DuplexConnection> connection,
      ProtocolVersion protocolVersion);

  ~FramedDuplexConnection();

  void send(std::unique_ptr<folly::IOBuf>) override;

  void setInput(std::shared_ptr<DuplexConnection::Subscriber>) override;

  bool isFramed() const override {
    return true;
  }

  DuplexConnection* getConnection() {
    return inner_.get();
  }

 private:
  const std::unique_ptr<DuplexConnection> inner_;
  std::shared_ptr<FramedReader> inputReader_;
  const std::shared_ptr<ProtocolVersion> protocolVersion_;
};
} // namespace rsocket
