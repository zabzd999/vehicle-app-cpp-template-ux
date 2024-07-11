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

#ifndef VEHICLE_APP_SDK_SEATADJUSTER_EXAMPLE_H
#define VEHICLE_APP_SDK_SEATADJUSTER_EXAMPLE_H

#include "sdk/Status.h"
#include "sdk/VehicleApp.h"
#include "vehicle/Vehicle.hpp"

#include <memory>
#include <string>

namespace example {

/**
 * @brief Sample SeatAdjuster vehicle app.
 * @details The SeatAdjuster subscribes to a setPosition MQTT topic
 *      to listen for incoming requests to set the seat position and
 *      publishes it to a response topic.
 *
 *      It also subcribes to the VehicleDataBroker
 *      directly for updates of the
 *      driverseat position signal and publishes this
 *      information via another specific MQTT topic
 */
class SeatAdjusterApp : public velocitas::VehicleApp {
public:
    SeatAdjusterApp();

    /**
     * @brief Run when the vehicle app starts
     *
     */
    void onStart() override;

    /**
     * @brief Handle set position request from PubSub topic
     *
     * @param data  The JSON string received from PubSub topic.
     */
    void onSetDriverPositionRequestReceived(const std::string& data);

    void onSetCoDriverPositionRequestReceived(const std::string& data);

    /**
     * @brief Handle seat movement events from the VDB.
     *
     * @param dataPoints  The affected data points.
     */
    void onDriverSeatPositionChanged(const velocitas::DataPointReply& dataPoints);

    void onCoDriverSeatPositionChanged(const velocitas::DataPointReply& dataPoints);

    /**
     * @brief Handle errors which occurred during async invocation.
     *
     * @param status  The status which contains the error.
     */
    void onError(const velocitas::Status& status);
    void onErrorDatapoint(const velocitas::Status& status);
    void onErrorTopic(const velocitas::Status& status);

private:
    vehicle::Vehicle Vehicle;
};

} // namespace example

#endif // VEHICLE_APP_SDK_SEATADJUSTER_EXAMPLE_H
