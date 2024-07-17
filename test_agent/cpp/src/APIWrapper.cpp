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

#include <APIWrapper.h>

using namespace rapidjson;
using namespace uprotocol::v1;
using namespace std;

APIWrapper::APIWrapper(const std::string transportType) {
	// Log the creation of the APIWrapper with the specified transport type
	spdlog::info(
	    "APIWrapper::APIWrapper(), Creating APIWrapper with transport type: {}",
	    transportType);

	// Create the transport with the specified type
	// transportPtr_ = createTransport(transportType);
	// TODO:: TEMP solution to avoid compilation error
	uprotocol::v1::UUri def_src_uuri;
	def_src_uuri.set_authority_name("TestAgentCpp");
	def_src_uuri.set_ue_id(0x18000);
	def_src_uuri.set_ue_version_major(1);
	def_src_uuri.set_resource_id(0);

	transportPtr_ =
	    std::make_shared<uprotocol::test::UTransportMock>(def_src_uuri);

	// If the transport creation failed, log an error and exit
	if (nullptr == transportPtr_) {
		spdlog::error("APIWrapper::APIWrapper(), Failed to create transport");
		exit(1);
	}
}

APIWrapper::~APIWrapper() {}

void APIWrapper::sendToTestManager(const uprotocol::v1::UMessage& proto,
                                   const std::string& action,
                                   const std::string& strTest_id) const {}

void APIWrapper::sendToTestManager(const uprotocol::v1::UStatus& status,
                                   const std::string& action,
                                   const std::string& strTest_id) const {};

void APIWrapper::sendToTestManager(rapidjson::Document& doc,
                                   rapidjson::Value& jsonVal,
                                   const std::string action,
                                   const std::string& strTest_id) const {}

// std::shared_ptr<uprotocol::transport::UTransport>
// APIWrapper::createTransport(
//     const std::string& transportType) {
// 	// If the transport type is "socket", create a new SocketUTransport.
// 	if (transportType == "socket") {
// 		return std::make_shared<SocketUTransport>();
// 	}
// 	// If the transport type is "zenoh", create a new UpZenohClient.
// 	else if (transportType == "zenoh") {
// 		return uprotocol::client::UpZenohClient::instance(
// 		    BuildUAuthority().setName("cpp").build(), BuildUEntity()
// 		                                                  .setName("rpc.client")
// 		                                                  .setMajorVersion(1)
// 		                                                  .setId(1)
// 		                                                  .build());
// 	} else {
// 		// If the transport type is neither "socket" nor "zenoh", log an error
// 		// and return null.
// 		spdlog::error("Invalid transport type: {}", transportType);
// 		return nullptr;
// 	}
// }

UStatus APIWrapper::addHandleToUriCallbackMap(CommunicationVariantType&& handle,
                                              const UUri& uri) {
	// Insert into the map using emplace to avoid deleted move assignment
	uriCallbackMap_.emplace(uri.SerializeAsString(), std::move(handle));

	UStatus status;
	status.set_code(UCode::OK);
	return status;
}

UStatus APIWrapper::removeHandleOrProvideError(Document& jsonData) {
	auto uri = ProtoConverter::distToUri(jsonData[Constants::DATA],
	                                     jsonData.GetAllocator());

	return removeHandleOrProvideError(uri);
}

UStatus APIWrapper::removeHandleOrProvideError(const UUri& uri) {
	UStatus status;

	auto count = uriCallbackMap_.erase(uri.SerializeAsString());
	if (count == 0) {
		spdlog::error("APIWrapper::removeCallbackToMap, URI not found.");
		status.set_code(UCode::NOT_FOUND);
	}

	// Remove the rpc handles that are not valid
	rpcClientHandles_.erase(
	    std::remove_if(
	        rpcClientHandles_.begin(), rpcClientHandles_.end(),
	        [](const uprotocol::communication::RpcClient::InvokeHandle&
	               handle) { return !handle; }),
	    rpcClientHandles_.end());

	status.set_code(UCode::OK);

	return status;
}

UStatus APIWrapper::handleSendCommand(Document& jsonData) {
	// Create a v1 UMessage object.
	UMessage umsg;
	// Convert the jsonData to a proto message.
	ProtoConverter::dictToProto(jsonData[Constants::DATA], umsg,
	                            jsonData.GetAllocator());
	// Log the UMessage string.
	spdlog::info("APIWrapper::handleSendCommand(), umsg string is: {}",
	             umsg.DebugString());

	// Send the UMessage and return the status.
	return transportPtr_->send(umsg);
}

UStatus APIWrapper::handleRegisterListenerCommand(Document& jsonData) {
	// Create a v1 UUri object.
	auto uri = ProtoConverter::distToUri(jsonData[Constants::DATA],
	                                     jsonData.GetAllocator());

	auto RESPONSE_ON_RECEIVE = Constants::RESPONSE_ON_RECEIVE;

	// Register the lambda function as a listener for messages on the specified
	// URI.
	auto result = transportPtr_->registerListener(
	    // sink_uri =
	    uri,
	    // callback =
	    [this, RESPONSE_ON_RECEIVE](const UMessage& transportUMessage) {
		    spdlog::info("APIWrapper::onReceive(), received.");

		    // Send the message to the Test Manager with a predefined response.
		    sendToTestManager(transportUMessage, RESPONSE_ON_RECEIVE);
	    });

	return result.has_value()
	           ? addHandleToUriCallbackMap(std::move(result).value(), uri)
	           : result.error();
}

UStatus APIWrapper::handleInvokeMethodCommand(Document& jsonData) {
	// Get the data and test ID from the JSON document.
	Value& data = jsonData[Constants::DATA];
	std::string strTest_id = jsonData[Constants::TEST_ID].GetString();
	UStatus status;

	auto format = ProtoConverter::distToUPayFormat(
	    data[Constants::ATTRIBUTES][Constants::FORMAT]);

	if (!format.has_value()) {
		spdlog::error(
		    "APIWrapper::handleInvokeMethodCommand(), Invalid format "
		    "received.");
		status.set_code(UCode::NOT_FOUND);
		status.set_message("Invalid payload format received in the request.");
		return status;
	}

	// Build payload
	std::string valueStr = std::string(data[Constants::PAYLOAD].GetString());

	uprotocol::datamodel::builder::Payload payload(valueStr, format.value());

	// Create a UUri object.
	auto uri = ProtoConverter::distToUri(
	    data[Constants::ATTRIBUTES][Constants::SINK], jsonData.GetAllocator());

	// Log the UUri string.
	spdlog::debug(
	    "APIWrapper::handleInvokeMethodCommand(), UUri in string format is :  "
	    "{}",
	    uri.DebugString());

	std::chrono::milliseconds ttl = std::chrono::milliseconds(10000);

	// Serialize the URI
	std::string serializedUri = uri.SerializeAsString();

	// Attempt to find the range of elements with the matching key
	auto range = uriCallbackMap_.equal_range(serializedUri);

	// Flag to track the RpcClient
	bool rpcClientExists = false;

	if (range.first != uriCallbackMap_.end()) {
		// Iterate over the value to check for an existing RpcClient type
		for (auto it = range.first; it != range.second; ++it) {
			if (std::holds_alternative<uprotocol::communication::RpcClient>(
			        it->second)) {
				rpcClientExists = true;
				break;
			}
		}
	}

	// If no RpcClient exists for the given URI, create and add one
	if (!rpcClientExists) {
		// Create RPC Client object
		auto rpcClient = uprotocol::communication::RpcClient(
		    transportPtr_, std::move(uri), UPriority::UPRIORITY_CS4, ttl,
		    format);

		// Add the new RpcClient to the map, ignoring the return value
		std::ignore = addHandleToUriCallbackMap(std::move(rpcClient), uri);
	}

	// Define a lambda function for handling received messages.
	// This function logs the reception of a message and forwards it to the TM.
	auto callBack = [this, &strTest_id](auto responseOrError) {
		spdlog::info("APIWrapper::onReceive(), received.");

		if (!responseOrError.has_value()) {
			auto& status = responseOrError.error();
			spdlog::error("APIWrapper rpc callback, Error received: {}",
			              status.message());
			// Send the message to the Test Manager.
			sendToTestManager(std::move(responseOrError).error(),
			                  Constants::INVOKE_METHOD_COMMAND, strTest_id);
		}
		// Send the message to the Test Manager.
		sendToTestManager(std::move(responseOrError).value(),
		                  Constants::INVOKE_METHOD_COMMAND, strTest_id);
	};

	// reteive the rpc client as it is already added or existing
	auto& rpcClient = std::get<uprotocol::communication::RpcClient>(
	    uriCallbackMap_.find(serializedUri)->second);

	// Invoke the method
	auto handle =
	    rpcClient.invokeMethod(std::move(payload), std::move(callBack));

	rpcClientHandles_.push_back(std::move(handle));

	status.set_code(UCode::OK);
	return status;
}

UStatus APIWrapper::handleRpcServerCommand(Document& jsonData) {
	// Get the data and test ID from the JSON document.
	Value& data = jsonData[Constants::DATA];
	std::string strTest_id = jsonData[Constants::TEST_ID].GetString();
	UStatus status;

	auto format = ProtoConverter::distToUPayFormat(
	    data[Constants::ATTRIBUTES][Constants::FORMAT]);

	if (!format.has_value()) {
		spdlog::error(
		    "APIWrapper::handleRpcServerCommand(), Invalid format received.");
		status.set_code(UCode::NOT_FOUND);
		status.set_message("Invalid payload format received in the request.");
		return status;
	}

	// Build payload
	std::string valueStr = std::string(data[Constants::PAYLOAD].GetString());

	uprotocol::datamodel::builder::Payload payload(valueStr, format.value());

	// Remove the payload from the data to make it a proper UUri.
	data.RemoveMember("payload");

	// Create a UUri object.
	auto uri = ProtoConverter::distToUri(data, jsonData.GetAllocator());

	// Log the UUri string.
	spdlog::debug(
	    "APIWrapper::handleRpcServerCommand(), UUri in string format is :  "
	    "{}",
	    uri.DebugString());

	auto rpcServerCallback = [payload](const uprotocol::v1::UMessage& message)
	    -> std::optional<uprotocol::datamodel::builder::Payload> {
		return payload;
	};

	// Serialize the URI
	std::string serializedUri = uri.SerializeAsString();

	// Attempt to find the range of elements with the matching key
	auto range = uriCallbackMap_.equal_range(serializedUri);

	if (range.first != uriCallbackMap_.end()) {
		// Iterate over the value to check for an existing RpcClient type
		for (auto it = range.first; it != range.second; ++it) {
			if (std::holds_alternative<
			        std::unique_ptr<uprotocol::communication::RpcServer>>(
			        it->second)) {
				status.set_code(UCode::ALREADY_EXISTS);
				status.set_message(
				    "RPC Server already exists for the given URI.");
				return status;
			}
		}
	}

	// Create RPC Client object
	auto rpcServer = uprotocol::communication::RpcServer::create(
	    transportPtr_, uri, rpcServerCallback, format);

	return rpcServer.has_value()
	           ? addHandleToUriCallbackMap(std::move(rpcServer).value(), uri)
	           : rpcServer.error();
}

UStatus APIWrapper::handlePublisherCommand(Document& jsonData) {
	// Get the data and test ID from the JSON document.
	Value& data = jsonData[Constants::DATA];
	std::string strTest_id = jsonData[Constants::TEST_ID].GetString();

	auto format = ProtoConverter::distToUPayFormat(
	    data[Constants::ATTRIBUTES][Constants::FORMAT]);

	if (!format.has_value()) {
		spdlog::error(
		    "APIWrapper::handlePublisherCommand(), Invalid format received.");
		UStatus status;
		status.set_code(UCode::NOT_FOUND);
		status.set_message("Invalid payload format received in the request.");
		return status;
	}

	// Build payload
	auto valueStr = std::string(data[Constants::PAYLOAD].GetString());
	uprotocol::datamodel::builder::Payload payload(valueStr, format.value());

	// Remove the payload from the data to make it a proper UUri.
	data.RemoveMember("payload");

	// Create a UUri object.
	auto uri = ProtoConverter::distToUri(data, jsonData.GetAllocator());

	// Log the UUri string.
	spdlog::debug(
	    "APIWrapper::handleInvokeMethodCommand(), UUri in string format is :  "
	    "{}",
	    uri.DebugString());

	std::chrono::milliseconds ttl = std::chrono::milliseconds(10000);
	// Create RPC Client object
	uprotocol::communication::Publisher publisher(transportPtr_, std::move(uri),
	                                              format.value());

	auto status = publisher.publish(std::move(payload));

	return status;
}

UStatus APIWrapper::handleSubscriberCommand(Document& jsonData) {
	// Create a UUri object.
	auto uri = ProtoConverter::distToUri(jsonData[Constants::DATA],
	                                     jsonData.GetAllocator());

	// Get the test ID from the JSON data.
	std::string strTest_id = jsonData[Constants::TEST_ID].GetString();

	// Define a lambda function for handling received messages.
	// This function logs the reception of a message and forwards it to the TM.
	auto callBack = [this, &strTest_id](const UMessage response) {
		spdlog::info("Subscribe response received.");

		// Send the message to the Test Manager.
		sendToTestManager(response, Constants::SUBSCRIBER_COMMAND, strTest_id);
	};

	// Create a subscriber object.
	auto subscriber = uprotocol::communication::Subscriber::subscribe(
	    transportPtr_, uri, std::move(callBack));

	// Add the subscriber object to the URI callback map.
	return subscriber.has_value()
	           ? addHandleToUriCallbackMap(std::move(subscriber).value(), uri)
	           : subscriber.error();
}
