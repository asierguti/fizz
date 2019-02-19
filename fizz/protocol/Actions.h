/*
 *  Copyright (c) 2018-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <fizz/protocol/KeyScheduler.h>
#include <fizz/protocol/Types.h>
#include <fizz/record/RecordLayer.h>
#include <folly/ExceptionWrapper.h>
#include <folly/io/IOBuf.h>
#include <folly/io/async/AsyncTransport.h>
#include <folly/small_vector.h>
#include <vector>

namespace fizz {

/**
 * Application data to deliver to the application.
 */
struct DeliverAppData {
  std::unique_ptr<folly::IOBuf> data;
};

/**
 * Raw data that must be written to the transport.
 * callback: The callback that should be invoked after the write has finished.
 *           This is usually when the kernel has accepted the buffer in the
 *           case of TCP.
 * contents: The TLS records that need to be written. Each TLSContent object
 *           can represent several TLS records at a certain encryption level /
 *           content type.
 * flags   : The flags to use when writing the contents to the socket.
 */
struct WriteToSocket {
  folly::AsyncTransportWrapper::WriteCallback* callback{nullptr};
#if defined(FOLLY_MOBILE)
  std::vector<TLSContent> contents;
#else
  folly::small_vector<TLSContent, 4> contents;
#endif
  folly::WriteFlags flags{folly::WriteFlags::NONE};
};

/**
 * Reports that a fatal error has occured on the connection.
 */
struct ReportError {
  folly::exception_wrapper error;

  explicit ReportError(const std::string& errorMsg)
      : error(folly::make_exception_wrapper<std::runtime_error>(errorMsg)) {}

  explicit ReportError(folly::exception_wrapper e) : error(std::move(e)) {}
};

/**
 * Reports that more data is needed to progress. waitForData() should be
 * called.
 */
struct WaitForData {};

/**
 * New secret available. This event is triggered whenever the TLS layer derives
 * new keys. This should not normally be used unless logging keys or not using
 * the TLS record layer.
 */
struct SecretAvailable {
  DerivedSecret secret;
  SecretAvailable(DerivedSecret secretIn) : secret(std::move(secretIn)) {}
};

/**
 * Reports that end of the TLS session. While the end of a TLS session
 * typically implies the termination of a network transport, this is not
 * mandatory, and it is possible to continue reusing the transport afterwards.
 */
struct EndOfData {
  EndOfData() = default;
  explicit EndOfData(std::unique_ptr<folly::IOBuf> postData)
      : postTlsData(std::move(postData)) {}

  // Any data we read from the connection after the end of the TLS session.
  std::unique_ptr<folly::IOBuf> postTlsData;
};

namespace detail {
template <typename ActionsType>
void addAction(ActionsType& /*acts*/) {}

template <typename ActionsType, typename ActionType, typename... Args>
void addAction(ActionsType& acts, ActionType&& thisAct, Args&&... act) {
  acts.emplace_back(std::forward<ActionType>(thisAct));
  addAction(acts, std::forward<Args>(act)...);
}
} // namespace detail
} // namespace fizz
