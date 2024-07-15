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

#ifndef _TEST_AGENT_H_
#define _TEST_AGENT_H_

#include <Constants.h>
//#include <SocketUTransport.h>
//#include <up-client-zenoh-cpp/client/upZenohClient.h>
// TODO:: Remote transport mock include
#include <UTransportMock.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <up-cpp/communication/Publisher.h>
#include <up-cpp/communication/RpcClient.h>
#include <up-cpp/communication/RpcServer.h>
#include <up-cpp/communication/Subscriber.h>

#include <variant>

#include "ProtoConverter.h"

using FunctionType =
    std::variant<std::function<uprotocol::v1::UStatus(rapidjson::Document&)>,
                 std::function<void(rapidjson::Document&)>>;

using CommunicationVariantType =
    std::variant<uprotocol::transport::UTransport::ListenHandle,
                 std::unique_ptr<uprotocol::communication::Subscriber>,
                 uprotocol::communication::RpcClient,
                 std::unique_ptr<uprotocol::communication::RpcServer>>;

// TODO:: Come back to this later let try different approach now
// using ResultType = uprotocol::utils::Expected<CommunicationVariantType,
// uprotocol::v1::UStatus>;

/// @class TestAgent
/// @brief Represents a test agent that communicates with a test manager.
///
/// The TestAgent class is responsible for connecting to a test manager, sending
/// and receiving messages, and handling various commands. It inherits from the
/// UListener class to handle incoming messages.

class TestAgent {
public:
	/// @brief Constructs a TestAgent object with the specified transport type.
	/// @param[in] transportType The type of transport to be used by the agent.
	TestAgent(const std::string transportType);

	/// @brief Destroys the TestAgent object.
	~TestAgent();

	/// @brief Connects the agent to the test manager.
	/// @return True if the connection is successful, false otherwise.
	bool socketConnect();

	/// @brief Receives data from the test manager.
	void receiveFromTM();

	/// @brief Sends a message to the test manager.
	/// @param[in] proto The message to be sent.
	/// @param[in] action The action associated with the message.
	/// @param[in] strTest_id The ID of the test (optional).
	void sendToTestManager(const uprotocol::v1::UMessage& proto,
	                       const std::string& action,
	                       const std::string& strTest_id = "") const;

	/// @brief Sends a message to the test manager.
	/// @param[in,out] doc The JSON document to be sent.
	/// @param[in,out] jsonVal The JSON value to be sent.
	/// @param[in] action The action associated with the message.
	/// @param[in] strTest_id The ID of the test (optional).
	void sendToTestManager(rapidjson::Document& doc, rapidjson::Value& jsonVal,
	                       const std::string action,
	                       const std::string& strTest_id = "") const;

private:
	// The socket used for communication with the test manager.
	int clientSocket_;
	// The address of the test manager.
	struct sockaddr_in mServerAddress_;
	// The transport layer used for communication.
	std::shared_ptr<uprotocol::transport::UTransport> transportPtr_;
	// The map of action handlers.
	std::unordered_map<std::string, FunctionType> actionHandlers_;

	// The map of uri to callback handle
	std::unordered_multimap<std::string, CommunicationVariantType>
	    uriCallbackMap_;

	std::vector<uprotocol::communication::RpcClient::InvokeHandle>
	    rpcClientHandles_;

	/// @brief Processes the received message.
	/// @param[in,out] jsonData The JSON data of the received message.
	void processMessage(rapidjson::Document& jsonData);

	/// @brief Disconnects the agent from the test manager.
	/// @return The status of the disconnection.
	void socketDisconnect();

	uprotocol::v1::UStatus addHandleToUriCallbackMap(
	    CommunicationVariantType&& handel, const uprotocol::v1::UUri& uri);

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

	/// @brief Handles the "unregisterListener" command received from the test
	/// manager.
	/// @param[in,out] jsonData The JSON data of the command.
	/// @return The status of the command handling.
	uprotocol::v1::UStatus handleUnregisterListenerCommand(
	    rapidjson::Document& jsonData);

	/// @brief Handles the "invokeMethod" command received from the test
	/// manager.
	/// @param[in,out] jsonData The JSON data of the command.
	uprotocol::v1::UStatus handleRpcClientCommand(
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

	/// @brief Creates a transport layer object based on the specified transport
	/// type.
	/// @param[in] transportType The type of transport to be created.
	/// @return A shared pointer to the created transport layer object.
	// std::shared_ptr<uprotocol::transport::UTransport> createTransport(
	//     const std::string& transportType);

	/// @brief Writes data to the test manager socket.
	/// @param[in,out] responseDoc The JSON document containing the response
	/// data.
	/// @param[in] action The action associated with the response.
	void writeDataToTMSocket(rapidjson::Document& responseDoc,
	                         const std::string action) const;

	/// @brief Creates a string value for a RapidJSON document.
	/// @param[in,out] doc The RapidJSON document to which the string value will
	/// be added.
	/// @param[in] data The string data to be converted into a RapidJSON value.
	/// @return A rapidjson::Value object containing the string data.
	/// This function takes a RapidJSON document and a string as parameters,
	/// creates a RapidJSON value from the string, and returns it.
	/// The returned value can be added to a RapidJSON document as either a key
	/// or a value.
	rapidjson::Value createRapidJsonStringValue(rapidjson::Document& doc,
	                                            const std::string& data) const;
};

#endif  //_TEST_AGENT_H_
