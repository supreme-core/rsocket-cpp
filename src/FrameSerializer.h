// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <memory>
#include <string>

#include "Frame.h"

namespace reactivesocket {

// interface separating serialization/deserialization of ReactiveSocket frames
class FrameSerializer {
 public:
  virtual ~FrameSerializer() = default;

  virtual std::string protocolVersion() = 0;

  static std::unique_ptr<FrameSerializer> createFrameSerializer(
      std::string protocolVersion);

  virtual std::unique_ptr<folly::IOBuf> serializeOut(
      Frame_REQUEST_STREAM&&) = 0;
  virtual std::unique_ptr<folly::IOBuf> serializeOut(Frame_REQUEST_SUB&&) = 0;
  virtual std::unique_ptr<folly::IOBuf> serializeOut(
      Frame_REQUEST_CHANNEL&&) = 0;
  virtual std::unique_ptr<folly::IOBuf> serializeOut(
      Frame_REQUEST_RESPONSE&&) = 0;
  virtual std::unique_ptr<folly::IOBuf> serializeOut(Frame_REQUEST_FNF&&) = 0;
  virtual std::unique_ptr<folly::IOBuf> serializeOut(Frame_REQUEST_N&&) = 0;
  virtual std::unique_ptr<folly::IOBuf> serializeOut(Frame_METADATA_PUSH&&) = 0;
  virtual std::unique_ptr<folly::IOBuf> serializeOut(Frame_CANCEL&&) = 0;
  virtual std::unique_ptr<folly::IOBuf> serializeOut(Frame_RESPONSE&&) = 0;
  virtual std::unique_ptr<folly::IOBuf> serializeOut(Frame_ERROR&&) = 0;
  virtual std::unique_ptr<folly::IOBuf> serializeOut(
      Frame_KEEPALIVE&&,
      bool) = 0;
  virtual std::unique_ptr<folly::IOBuf> serializeOut(Frame_SETUP&&) = 0;
  virtual std::unique_ptr<folly::IOBuf> serializeOut(Frame_LEASE&&) = 0;
  virtual std::unique_ptr<folly::IOBuf> serializeOut(Frame_RESUME&&) = 0;
  virtual std::unique_ptr<folly::IOBuf> serializeOut(Frame_RESUME_OK&&) = 0;

  virtual bool deserializeFrom(
      Frame_REQUEST_STREAM&,
      std::unique_ptr<folly::IOBuf>) = 0;
  virtual bool deserializeFrom(
      Frame_REQUEST_SUB&,
      std::unique_ptr<folly::IOBuf>) = 0;
  virtual bool deserializeFrom(
      Frame_REQUEST_CHANNEL&,
      std::unique_ptr<folly::IOBuf>) = 0;
  virtual bool deserializeFrom(
      Frame_REQUEST_RESPONSE&,
      std::unique_ptr<folly::IOBuf>) = 0;
  virtual bool deserializeFrom(
      Frame_REQUEST_FNF&,
      std::unique_ptr<folly::IOBuf>) = 0;
  virtual bool deserializeFrom(
      Frame_REQUEST_N&,
      std::unique_ptr<folly::IOBuf>) = 0;
  virtual bool deserializeFrom(
      Frame_METADATA_PUSH&,
      std::unique_ptr<folly::IOBuf>) = 0;
  virtual bool deserializeFrom(
      Frame_CANCEL&,
      std::unique_ptr<folly::IOBuf>) = 0;
  virtual bool deserializeFrom(
      Frame_RESPONSE&,
      std::unique_ptr<folly::IOBuf>) = 0;
  virtual bool deserializeFrom(Frame_ERROR&, std::unique_ptr<folly::IOBuf>) = 0;
  virtual bool deserializeFrom(
      Frame_KEEPALIVE&,
      std::unique_ptr<folly::IOBuf>) = 0;
  virtual bool deserializeFrom(Frame_SETUP&, std::unique_ptr<folly::IOBuf>) = 0;
  virtual bool deserializeFrom(Frame_LEASE&, std::unique_ptr<folly::IOBuf>) = 0;
  virtual bool deserializeFrom(
      Frame_RESUME&,
      std::unique_ptr<folly::IOBuf>) = 0;
  virtual bool deserializeFrom(
      Frame_RESUME_OK&,
      std::unique_ptr<folly::IOBuf>) = 0;
};

} // reactivesocket
