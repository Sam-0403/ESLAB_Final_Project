#ifndef GATT_CLIENT_MODULE_H
#define GATT_CLIENT_MODULE_H

#include "events/EventQueue.h"
#include "platform/NonCopyable.h"

#include "ble/GattClient.h"

#include "gatt_client_process.h"
#include "mbed-trace/mbed_trace.h"

/* Handle discovery of the GATT server.
 *
 * First the GATT server is discovered in its entirety then each readable
 * characteristic is read and the client register to characteristic
 * notifications or indication when available. The client report server
 * indications and notification until the connection end.*/
class GattClientModule : private mbed::NonCopyable<GattClientModule>, public GattClient::EventHandler
{
    // Internal typedef to this class type.
    // It is used as a shorthand to pass member function as callbacks.
    typedef GattClientModule Self;

    typedef CharacteristicDescriptorDiscovery::DiscoveryCallbackParams_t DiscoveryCallbackParams_t;
    typedef CharacteristicDescriptorDiscovery::TerminationCallbackParams_t TerminationCallbackParams_t;
    typedef DiscoveredCharacteristic::Properties_t Properties_t;

public:
    /* Construct an empty client process.
     *
     * The function start() shall be called to initiate the discovery process.*/
    GattClientModule();

    ~GattClientModule();

    void setup(Semaphore &sem, Mutex &mutex, ConditionVariable &cond, char* buffer, bool &isConnected, char &mode);

    void start(BLE &ble, events::EventQueue &event_queue);

    /* Start the discovery process.
     *
     * @param[in] client The GattClient instance which will discover the distant
     * GATT server.
     * @param[in] connection_handle Reference of the connection to the GATT
     * server which will be discovered.*/
    void start_discovery(BLE &ble_interface, events::EventQueue &event_queue, const ble::ConnectionCompleteEvent &event);

    /* Stop the discovery process and clean the instance.*/
    void stop();

    ble::connection_handle_t get_connection_handle_t(){
        return _connection_handle;
    }

private:
    /* Implementation of GattClient::EventHandler::onAttMtuChange event*/
    virtual void onAttMtuChange(ble::connection_handle_t connectionHandle, uint16_t attMtuSize);

private:
    // Service and characteristic discovery process.

    /* Handle services discovered.
     *
     * The GattClient invokes this function when a service has been discovered.
     *
     * @see GattClient::launchServiceDiscovery*/
    void when_service_discovered(const DiscoveredService *discovered_service);

    /* Handle characteristics discovered.
     *
     * The GattClient invoke this function when a characteristic has been
     * discovered.
     *
     * @see GattClient::launchServiceDiscovery*/
    void when_characteristic_discovered(const DiscoveredCharacteristic *discovered_characteristic);

    /* Handle termination of the service and characteristic discovery process.
     *
     * The GattClient invokes this function when the service and characteristic
     * discovery process ends.
     *
     * @see GattClient::onServiceDiscoveryTermination*/
    void when_service_discovery_ends(ble::connection_handle_t connection_handle);

    // Processing of characteristics based on their properties.

    /* Process the characteristics discovered.
     *
     * - If the characteristic is readable then read its value and print it. Then
     * - If the characteristic can emit notification or indication then discover
     * the characteristic CCCD and subscribe to the server initiated event.
     * - Otherwise skip the characteristic processing.*/
    void process_next_characteristic(void);

    /* Initate the read of the characteristic in input.
     *
     * The completion of the operation will happens in when_characteristic_read()*/
    void read_characteristic(const DiscoveredCharacteristic &characteristic);

    /* Handle the reception of a read response.
     *
     * If the characteristic can emit notification or indication then start the
     * discovery of the the characteristic descriptors then subscribe to the
     * server initiated event by writing the CCCD discovered. Otherwise start
     * the processing of the next characteristic discovered in the server.*/
    void when_characteristic_read(const GattReadCallbackParams *read_event);

    /* Initiate the discovery of the descriptors of the characteristic in input.
     *
     * When a descriptor is discovered, the function when_descriptor_discovered
     * is invoked.*/
    void discover_descriptors(const DiscoveredCharacteristic &characteristic);

    /* Handle the discovery of the characteristic descriptors.
     *
     * If the descriptor found is a CCCD then stop the discovery. Once the
     * process has ended subscribe to server initiated events by writing the
     * value of the CCCD.*/
    void when_descriptor_discovered(const DiscoveryCallbackParams_t* event);

    /* If a CCCD has been found subscribe to server initiated events by writing
     * its value.*/
    void when_descriptor_discovery_ends(const TerminationCallbackParams_t *event);

    /* Called when the CCCD has been written.*/
    void when_descriptor_written(const GattWriteCallbackParams* event);

    /* Print the updated value of the characteristic.
     *
     * This function is called when the server emits a notification or an
     * indication of a characteristic value the client has subscribed to.
     *
     * @see GattClient::onHVX()*/
    void when_characteristic_changed(const GattHVXCallbackParams* event);

    struct DiscoveredCharacteristicNode {
        DiscoveredCharacteristicNode(const DiscoveredCharacteristic &c) :
            value(c), next(nullptr) { }

        DiscoveredCharacteristic value;
        DiscoveredCharacteristicNode *next;
    };

    /* Add a discovered characteristic into the list of discovered characteristics.*/
    bool add_characteristic(const DiscoveredCharacteristic *characteristic);

    /* Clear the list of discovered characteristics.*/
    void clear_characteristics(void);

    /* Helper to construct an event handler from a member function of this
     * instance.*/
    template<typename ContextType>
    FunctionPointerWithContext<ContextType> as_cb(void (Self::*member)(ContextType context))
    {
        return makeFunctionPointer(this, member);
    }

    /* Print the value of a UUID.*/
    static void print_uuid(const UUID &uuid);

    /* Print the value of a characteristic properties.*/
    static void print_properties(const Properties_t &properties);

private:
    BLE *_ble = nullptr;
    events::EventQueue *_event_queue = nullptr;
    GattClient *_client = nullptr;
    Semaphore *_print_sem = nullptr;
    Mutex *_mutex = nullptr;
    ConditionVariable *_cond = nullptr;
    char* _buffer;
    bool* _isConnected;
    char* _mode;

    ble::connection_handle_t _connection_handle = 0;
    DiscoveredCharacteristicNode *_characteristics = nullptr;
    DiscoveredCharacteristicNode *_it = nullptr;
    GattAttribute::Handle_t _descriptor_handle = 0;
};

#endif