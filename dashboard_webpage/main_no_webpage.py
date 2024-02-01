import zmq
from data_collection import listen_to_channels, read_config, generate_address_to_channels, create_channel_info_mapping

def main():
    # Read config
    config = read_config("config.json")

    if not config:
        print("Error reading config.")
        return

    # Generate address to channels mapping
    address_to_channels = generate_address_to_channels(config)

    # Create channel info mapping
    channel_info_mapping = create_channel_info_mapping(address_to_channels)

    # Create shared data dictionary
    shared_data = {
        'address_to_channels': address_to_channels,
        'channel_info_mapping': channel_info_mapping,
    }

    # Run the listen_to_channels function in a single thread
    listen_to_channels(shared_data)

if __name__ == '__main__':
    main()
