import time
import json
import math

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
        self.last_receive_time = None
        self.latest_content = None

    def update_publish(self, data_size, content):
        self.total_publishes += 1
        self.total_data_size += data_size

        self.latest_content = content
        self.last_receive_time = time.time()

        if self.total_publishes == 1:
            self.start_time = time.time()
        else:
            self._calculate_rates()

        # Update the average data size
        self._calculate_average_data_size()

    def _calculate_rates(self):
        current_time = time.time()
        time_elapsed = current_time - self.start_time

        if time_elapsed > 0:
            self.rate_publishes = self.total_publishes / time_elapsed
            self.rate_data = self.total_data_size / time_elapsed

    def _calculate_average_data_size(self):
        if self.total_publishes > 0:
            self.average_data_size = self.total_data_size / self.total_publishes

    def _round_to_significant_digits(self, value, num_digits):
        if value == 0:
            return 0
        return round(value, -int(math.floor(math.log10(abs(value)))) + (num_digits - 1))

    def to_dict(self):
        return {
            'address': str(self.address),
            'channel_name': str(self.channel_name),
            'total_publishes': self.total_publishes,
            'total_data_size': self.total_data_size,
            'average_data_size': self._round_to_significant_digits(self.average_data_size, 4),
            'rate_publishes': self._round_to_significant_digits(self.rate_publishes, 4),
            'rate_data': self._round_to_significant_digits(self.rate_data, 4),
            'start_time': self.start_time,
            'last_receive_time': self.last_receive_time,
            'latest_content': json.loads(self.latest_content.decode('utf-8')) if self.latest_content else None
        }


    def __str__(self):
        return f"Channel Info for Address {self.address} and Channel {self.channel_name}:\n" \
               f"Total Publishes: {self.total_publishes}\n" \
               f"Total Data Size: {self.total_data_size} bytes\n" \
               f"Average Data Size: {self.average_data_size} bytes\n" \
               f"Publish Rate: {self.rate_publishes} publishes/s\n" \
               f"Data Rate: {self.rate_data} bytes/s\n" \
               f"Last Receive Time: {self.last_receive_time}\n" \
               f"Lastest Content: {self.latest_content}\n"
