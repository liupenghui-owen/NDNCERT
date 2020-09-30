/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2017-2020, Regents of the University of California.
 *
 * This file is part of ndncert, a certificate management system based on NDN.
 *
 * ndncert is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * ndncert is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received copies of the GNU General Public License along with
 * ndncert, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 *
 * See AUTHORS.md for complete list of ndncert authors and contributors.
 */

#ifndef NDNCERT_CLIENT_MODULE_HPP
#define NDNCERT_CLIENT_MODULE_HPP

#include "configuration.hpp"
#include "request-state.hpp"
#include "crypto-support/crypto-helper.hpp"

namespace ndn {
namespace ndncert {

// TODO
// For each CA item in Client.Conf, create a validator instance and initialize it with CA's cert
// The validator instance should be in CaConfigItem

class ClientModule : noncopyable
{
public:
  /**
   * @brief Error that can be thrown from ClientModule
   */
  class Error : public std::runtime_error
  {
  public:
    using std::runtime_error::runtime_error;
  };

public:
  ClientModule(security::v2::KeyChain& keyChain);

  ~ClientModule();

  ClientConfig&
  getClientConf()
  {
    return m_config;
  }

  Status
  getApplicationStatus() const
  {
    return m_status;
  }

  std::string
  getChallengeStatus() const
  {
    return m_challengeStatus;
  }

  shared_ptr<Interest>
  generateInfoInterest(const Name& caName);

  bool
  verifyInfoResponse(const Data& reply);

  /**
   * @brief Process the replied PROBE INFO Data packet
   * Warning: this function will add a new trust anchor into the application.
   * Please invoke this function only when reply can be fully trusted or the CA
   * can be verified in later challenge phase.
   */
  void
  addCaFromInfoResponse(const Data& reply);

  shared_ptr<Interest>
  generateProbeInterest(const CaConfigItem& ca, std::vector<std::tuple<std::string, std::string>>&& probeInfo);

  void
  onProbeResponse(const Data& reply);

  shared_ptr<Interest>
  generateNewInterest(const time::system_clock::TimePoint& notBefore,
                      const time::system_clock::TimePoint& notAfter,
                      const Name& identityName = Name());

  shared_ptr<Interest>
  generateRevokeInterest(const security::v2::Certificate& certificate);

  std::list<std::string>
  onNewRenewRevokeResponse(const Data& reply);

  shared_ptr<Interest>
  generateChallengeInterest(const Block& paramTLV);

  void
  onChallengeResponse(const Data& reply);

  shared_ptr<Interest>
  generateDownloadInterest();

  shared_ptr<Interest>
  generateCertFetchInterest();

  void
  onCertFetchResponse(const Data& reply);

  void
  endSession();

PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  ClientConfig m_config;
  security::v2::KeyChain& m_keyChain;

  CaConfigItem m_ca;
  security::Key m_key;
  Name m_identityName;

  std::string m_requestId = "";
  Status m_status = Status::NOT_STARTED;
  std::string m_challengeStatus = "";
  std::string m_challengeType = "";
  Name m_issuedCertName;
  std::list<std::string> m_challengeList;
  bool m_isCertInstalled = false;
  bool m_isNewlyCreatedIdentity = false;
  bool m_isNewlyCreatedKey = false;

  int m_remainingTries = 0;
  time::system_clock::TimePoint m_freshBefore;

  ECDHState m_ecdh;
  uint8_t m_aesKey[16] = {0};
};

} // namespace ndncert
} // namespace ndn

#endif // NDNCERT_CLIENT_MODULE_HPP
