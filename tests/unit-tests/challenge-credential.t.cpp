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

#include "challenge-module/challenge-credential.hpp"
#include "test-common.hpp"
#include <ndn-cxx/security/signing-helpers.hpp>
#include <ndn-cxx/util/io.hpp>

namespace ndn {
namespace ndncert {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(TestChallengeCredential, IdentityManagementFixture)

BOOST_AUTO_TEST_CASE(LoadConfig)
{
  ChallengeCredential challenge("./tests/unit-tests/challenge-credential.conf.test");
  BOOST_CHECK_EQUAL(challenge.CHALLENGE_TYPE, "Credential");

  challenge.parseConfigFile();
  BOOST_CHECK_EQUAL(challenge.m_trustAnchors.size(), 1);
  auto cert = challenge.m_trustAnchors.front();
  BOOST_CHECK_EQUAL(cert.getName(),
                    "/ndn/site1/KEY/%11%BC%22%F4c%15%FF%17/self/%FD%00%00%01Y%C8%14%D9%A5");
}

BOOST_AUTO_TEST_CASE(HandleChallengeRequest)
{
  // create trust anchor
  ChallengeCredential challenge("./tests/unit-tests/challenge-credential.conf.test");
  auto identity = addIdentity(Name("/trust"));
  auto key = identity.getDefaultKey();
  auto trustAnchor = key.getDefaultCertificate();
  challenge.parseConfigFile();
  challenge.m_trustAnchors.front() = trustAnchor;

  // create certificate request
  auto identityA = addIdentity(Name("/example"));
  auto keyA = identityA.getDefaultKey();
  auto certA = key.getDefaultCertificate();
  RequestState request(Name("/example"), "123", RequestType::NEW, Status::BEFORE_CHALLENGE, certA);

  // create requester's credential
  auto identityB = addIdentity(Name("/trust/cert"));
  auto keyB = identityB.getDefaultKey();
  auto credentialName = Name(keyB.getName()).append("Credential").appendVersion();
  security::v2::Certificate credential;
  credential.setName(credentialName);
  credential.setContent(keyB.getPublicKey().data(), keyB.getPublicKey().size());
  SignatureInfo signatureInfo;
  signatureInfo.setValidityPeriod(security::ValidityPeriod(system_clock::now(), system_clock::now() + time::minutes(1)));
  m_keyChain.sign(credential, signingByCertificate(trustAnchor).setSignatureInfo(signatureInfo));

  // using private key to sign cert request
  Name idSignatureName = credential.getKeyName();
  idSignatureName.append("request-id-signature").appendVersion();
  security::v2::Certificate idSignature;
  idSignature.setName(idSignatureName);
  idSignature.setContent(makeStringBlock(tlv::Content, "123"));
  m_keyChain.sign(idSignature, signingByCertificate(credential));

  std::stringstream ss;
  io::save<security::v2::Certificate>(idSignature, ss);
  auto checkCert = *(io::load<security::v2::Certificate>(ss));
  BOOST_CHECK_EQUAL(checkCert, idSignature);
  ss.str("");
  ss.clear();

  io::save<security::v2::Certificate>(idSignature, ss);
  std::string idSignatureStr = ss.str();
  ss.str("");
  ss.clear();

  io::save<security::v2::Certificate>(credential, ss);
  std::string credentialStr = ss.str();
  ss.str("");
  ss.clear();

  Block params = makeEmptyBlock(tlv_encrypted_payload);
  params.push_back(makeStringBlock(tlv_selected_challenge, "Credential"));
  params.push_back(makeStringBlock(tlv_parameter_key, ChallengeCredential::PARAMETER_KEY_CREDENTIAL_CERT));
  params.push_back(makeStringBlock(tlv_parameter_value, credentialStr));
  params.push_back(makeStringBlock(tlv_parameter_key, ChallengeCredential::PARAMETER_KEY_PROOF_OF_PRIVATE_KEY));
  params.push_back(makeStringBlock(tlv_parameter_value, idSignatureStr));
  params.encode();

  challenge.handleChallengeRequest(params, request);
  BOOST_CHECK(request.m_status == Status::PENDING);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace tests
}  // namespace ndncert
}  // namespace ndn
