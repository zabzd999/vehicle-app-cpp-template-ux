/**
 * Copyright (c) 2023-2024 Contributors to the Eclipse Foundation
 *
 * This program and the accompanying materials are made available under the
 * terms of the Apache License, Version 2.0 which is available at
 * https://www.apache.org/licenses/LICENSE-2.0.
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "SeatAdjusterApp.h"
#include "sdk/IPubSubClient.h"
#include "sdk/Logger.h"
#include "sdk/QueryBuilder.h"
#include "sdk/vdb/IVehicleDataBrokerClient.h"

#include <fmt/core.h>
#include <nlohmann/json.hpp>
#include <utility>

namespace example {

const auto TOPIC_REQUEST          = "seatadjuster/setPosition/request";
const auto TOPIC_REQUEST_Right    = "seatadjuster/setPosition/requestRight";
const auto TOPIC_RESPONSE         = "seatadjuster/setPosition/response";
const auto TOPIC_CURRENT_POSITION = "seatadjuster/currentPosition";

const auto JSON_FIELD_REQUEST_ID = "requestId";
const auto JSON_FIELD_POSITION   = "position";
const auto JSON_FIELD_STATUS     = "status";
const auto JSON_FIELD_MESSAGE    = "message";
const auto JSON_FIELD_RESULT     = "result";

const auto STATUS_OK   = 0;
const auto STATUS_FAIL = 1;

SeatAdjusterApp::SeatAdjusterApp()
    : VehicleApp(velocitas::IVehicleDataBrokerClient::createInstance("vehicledatabroker"),
                 velocitas::IPubSubClient::createInstance("SeatAdjusterApp")) {}

void SeatAdjusterApp::onStart() {
    // This method will be called by the SDK when the connection to the
    // Vehicle DataBroker is ready.
    velocitas::logger().info("Subscribe for data points!");

    // Here you can subscribe for the Vehicle Signals update and provide callbacks.
    subscribeDataPoints(
        velocitas::QueryBuilder::select(Vehicle.Cabin.Seat.Row1.DriverSide.Position).build())
        ->onItem([this](auto&& item) { onSeatPositionChanged(std::forward<decltype(item)>(item)); })
        ->onError(
            [this](auto&& status) { onErrorDatapoint(std::forward<decltype(status)>(status)); });

    // ... and, unlike Python, you have to manually subscribe to pub/sub topics
    subscribeToTopic(TOPIC_REQUEST)
        ->onItem([this](auto&& item) {
            onSetPositionRequestReceived(std::forward<decltype(item)>(item));
        })
        ->onError([this](auto&& status) { onErrorTopic(std::forward<decltype(status)>(status)); });

    subscribeToTopic(TOPIC_REQUEST_Right)
        ->onItem([this](auto&& item) {
            onSetPositionRequestReceived(std::forward<decltype(item)>(item));
        })
        ->onError([this](auto&& status) { onErrorTopic(std::forward<decltype(status)>(status)); });
}

void SeatAdjusterApp::onSetPositionRequestReceived(const std::string& data) {
    // Callback is executed whenever a message is received on the subscribed topic
    // The data parameter contains the message payload
    // Payload format: {"requestId": 1, "position": 1}

    // Use the logger with the preferred log level (e.g. debug, info, error, etc)
    velocitas::logger().debug("position request: \"{}\"", data);

    // Parse the received JSON data
    const auto jsonData = nlohmann::json::parse(data);

    // Check if the received JSON data contains the required fields
    if (!jsonData.contains(JSON_FIELD_POSITION)) {
        const auto errorMsg = fmt::format("No position specified");
        velocitas::logger().error(errorMsg);

        nlohmann::json respData({{JSON_FIELD_REQUEST_ID, jsonData[JSON_FIELD_REQUEST_ID]},
                                 {JSON_FIELD_STATUS, STATUS_FAIL},
                                 {JSON_FIELD_MESSAGE, errorMsg}});
        publishToTopic(TOPIC_RESPONSE, respData.dump());
        return;
    }

    const auto desiredSeatPosition = jsonData[JSON_FIELD_POSITION].get<int>();
    const auto requestId           = jsonData[JSON_FIELD_REQUEST_ID].get<int>();

    nlohmann::json respData({{JSON_FIELD_REQUEST_ID, requestId}, {JSON_FIELD_RESULT, {}}});

    const auto vehicleSpeed = Vehicle.Speed.get()->await().value();

    // Check if the vehicle is not moving
    if (vehicleSpeed == 0) {
        // Move the seat to the desired position
        Vehicle.Cabin.Seat.Row1.DriverSide.Position.set(desiredSeatPosition)->await();

        respData[JSON_FIELD_RESULT][JSON_FIELD_STATUS] = STATUS_OK;
        respData[JSON_FIELD_RESULT][JSON_FIELD_MESSAGE] =
            fmt::format("Set Seat position to: {}", desiredSeatPosition);
    } else {
        const auto errorMsg = fmt::format(
            "Not allowed to move seat because vehicle speed is {} and not 0", vehicleSpeed);
        velocitas::logger().info(errorMsg);

        respData[JSON_FIELD_RESULT][JSON_FIELD_STATUS]  = STATUS_FAIL;
        respData[JSON_FIELD_RESULT][JSON_FIELD_MESSAGE] = errorMsg;
    }

    // Publish the response to the MQTT topic
    publishToTopic(TOPIC_RESPONSE, respData.dump());
}

void SeatAdjusterApp::onSeatPositionChanged(const velocitas::DataPointReply& dataPoints) {
    // Callback is executed whenever the subscribed datapoints are updated
    // The dataPoints parameter contains the updated datapoints
    nlohmann::json jsonResponse;
    try {
        // Get the current seat position
        const auto seatPositionValue =
            dataPoints.get(Vehicle.Cabin.Seat.Row1.DriverSide.Position)->value();
        jsonResponse[JSON_FIELD_POSITION] = seatPositionValue;
    } catch (std::exception& exception) {
        velocitas::logger().warn("Unable to get Current Seat Position, Exception: {}",
                                 exception.what());
        jsonResponse[JSON_FIELD_STATUS]  = STATUS_FAIL;
        jsonResponse[JSON_FIELD_MESSAGE] = exception.what();
    }

    // Publish the current seat position to the MQTT topic
    publishToTopic(TOPIC_CURRENT_POSITION, jsonResponse.dump());
}

// Error handling methods
void SeatAdjusterApp::onError(const velocitas::Status& status) {
    velocitas::logger().error("Error occurred during async invocation: {}", status.errorMessage());
}

void SeatAdjusterApp::onErrorDatapoint(const velocitas::Status& status) {
    velocitas::logger().error("Datapoint: Error occurred during async invocation: {}",
                              status.errorMessage());
}
void SeatAdjusterApp::onErrorTopic(const velocitas::Status& status) {
    velocitas::logger().error("Topic: Error occurred during async invocation: {}",
                              status.errorMessage());
}
} // namespace example