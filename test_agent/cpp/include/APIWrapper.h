// SPDX-FileCopyrightText: 2024 Contributors to the Eclipse Foundation
//
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
//
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
//
// SPDX-License-Identifier: Apache-2.0

#ifndef _API_WRAPPER_H_
#define _API_WRAPPER_H_

#include <Constants.h>
//#include <SocketUTransport.h>
//#include <up-client-zenoh-cpp/client/upZenohClient.h>
// TODO:: Remote transport mock include
#include <UTransportMock.h>
#include <up-cpp/communication/Publisher.h>
#include <up-cpp/communication/RpcClient.h>
#include <up-cpp/communication/RpcServer.h>
#include <up-cpp/communication/Subscriber.h>

#include <variant>

#include "ProtoConverter.h"

using CommunicationVariantType =
    std::variant<uprotocol::transport::UTransport::ListenHandle,
                 std::unique_ptr<uprotocol::communication::Subscriber>,
                 uprotocol::communication::RpcClient,
                 std::unique_ptr<uprotocol::communication::RpcServer>>;

/// @class TestAgent
/// @brief Represents a test agent that communicates with a test manager.
///
/// The TestAgent class is responsible for connecting to a test manager, sending
/// and receiving messages, and handling various commands. It inherits from the
/// UListener class to handle incoming messages.

class APIWrapper {
public:
	/// @brief Constructs a TestAgent object with the specified transport type.
	/// @param[in] transportType The type of transport to be used by the agent.
	APIWrapper(const std::string transportType);

	/// @brief Destroys the TestAgent object.
	virtual ~APIWrapper();

	/// @brief Sends a message to the test manager.
	/// @param[in] proto The message to be sent.
	/// @param[in] action The action associated with the message.
	/// @param[in] strTest_id The ID of the test (optional).
	virtual void sendToTestManager(const uprotocol::v1::UMessage& proto,
	                               const std::string& action,
	                               const std::string& strTest_id = "") const;

	/// @brief Sends a message to the test manager.
	/// @param[in] UStatus The status to be sent.
	/// @param[in] action The action associated with the message.
	/// @param[in] strTest_id The ID of the test (optional).
	virtual void sendToTestManager(const uprotocol::v1::UStatus& status,
	                               const std::string& action,
	                               const std::string& strTest_id = "") const;

	/// @brief Sends a message to the test manager.
	/// @param[in,out] doc The JSON document to be sent.
	/// @param[in,out] jsonVal The JSON value to be sent.
	/// @param[in] action The action associated with the message.
	/// @param[in] strTest_id The ID of the test (optional).
	virtual void sendToTestManager(rapidjson::Document& doc,
	                               rapidjson::Value& jsonVal,
	                               const std::string action,
	                               const std::string& strTest_id = "") const;

	uprotocol::v1::UStatus removeHandleOrProvideError(
	    rapidjson::Document& jsonData);

	uprotocol::v1::UStatus removeHandleOrProvideError(
	    const uprotocol::v1::UUri& uri);

	/// @brief Handles the "sendCommand" command received from the test manager.
	/// @param[in,out] jsonData The JSON data of the command.
	/// @return The status of the command handling.
	uprotocol::v1::UStatus handleSendCommand(rapidjson::Document& jsonData);

	/// @brief Handles the "registerListener" command received from the test
	/// manager.
	/// @param[in,out] jsonData The JSON data of the command.
	/// @return The status of the command handling.
	uprotocol::v1::UStatus handleRegisterListenerCommand(
	    rapidjson::Document& jsonData);

	/// @brief Handles the "invokeMethod" command received from the test
	/// manager.
	/// @details This function processes the "invokeMethod" command, which
	/// includes executing the specified method with the provided parameters.
	/// The command's JSON data contains necessary details such as the
	/// destination (attributes.sink), the format of the payload
	/// (attributes.payload_format), and the payload itself.
	/// @param[in,out] jsonData A rapidjson::Document object containing the
	/// command's JSON data. The JSON structure should include attributes.sink,
	/// attributes.payload_format, and the payload.
	/// @return uprotocol::v1::UStatus The status of the operation, indicating
	/// success or failure.
	uprotocol::v1::UStatus handleInvokeMethodCommand(
	    rapidjson::Document& jsonData);

	uprotocol::v1::UStatus handleRpcServerCommand(
	    rapidjson::Document& jsonData);

	/// @brief Handles the "publisher" command received from the test
	/// manager.
	/// @param[in,out] jsonData The JSON data of the command.
	/// @return The status of the command handling.
	uprotocol::v1::UStatus handlePublisherCommand(
	    rapidjson::Document& jsonData);

	/// @brief Handles the "deserializeUri" command received from the test
	/// manager.
	/// @param[in,out] jsonData The JSON data of the command.
	uprotocol::v1::UStatus handleSubscriberCommand(
	    rapidjson::Document& jsonData);

private:
	// The transport layer used for communication.
	std::shared_ptr<uprotocol::transport::UTransport> transportPtr_;

	// The map of uri to callback handle
	std::unordered_multimap<std::string, CommunicationVariantType>
	    uriCallbackMap_;

	std::vector<uprotocol::communication::RpcClient::InvokeHandle>
	    rpcClientHandles_;

	uprotocol::v1::UStatus addHandleToUriCallbackMap(
	    CommunicationVariantType&& handel, const uprotocol::v1::UUri& uri);

	/// @brief Creates a transport layer object based on the specified transport
	/// type.
	/// @param[in] transportType The type of transport to be created.
	/// @return A shared pointer to the created transport layer object.
	// std::shared_ptr<uprotocol::transport::UTransport> createTransport(
	//     const std::string& transportType);
};

#endif  //_API_WRAPPER_H_
