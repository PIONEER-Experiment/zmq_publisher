import time

class ChannelInfo:
    def __init__(self, address, channel_name):
        self.address = address
        self.channel_name = channel_name
        self.total_publishes = 0
        self.total_data_size = 0
        self.average_data_size = 0
        self.rate_publishes = 0
        self.rate_data = 0
        self.start_time = None

    def update_publish(self, data_size):
        self.total_publishes += 1
        self.total_data_size += data_size

        if self.total_publishes == 1:
            self.start_time = time.time()
        else:
            self.calculate_rates()
        
        print("Inside..." + self.__str__())

    def calculate_rates(self):
        current_time = time.time()
        time_elapsed = current_time - self.start_time

        if time_elapsed > 0:
            self.rate_publishes = self.total_publishes / time_elapsed
            self.rate_data = self.total_data_size / time_elapsed

    def __str__(self):
        return f"Channel Info for Address {self.address} and Channel {self.channel_name}:\n" \
               f"Total Publishes: {self.total_publishes}\n" \
               f"Total Data Size: {self.total_data_size} bytes\n" \
               f"Average Data Size: {self.average_data_size} bytes\n" \
               f"Publish Rate: {self.rate_publishes} publishes/s\n" \
               f"Data Rate: {self.rate_data} bytes/s"
