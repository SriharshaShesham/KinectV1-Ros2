#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from audio_common_msgs.msg import AudioData
import wave
import struct

class AudioSaver(Node):
    def __init__(self):
        super().__init__('audio_saver')
        self.subscription = self.create_subscription(
            AudioData,
            '/kinect/audio',
            self.listener_callback,
            10)
        self.wav_file = wave.open('kinect_capture.wav', 'wb')
        self.channels = 4          # Kinect v1 mic array
        self.sample_width = 2      # bytes (16-bit)
        self.sample_rate = 16000   # Hz
        self.wav_file.setnchannels(self.channels)
        self.wav_file.setsampwidth(self.sample_width)
        self.wav_file.setframerate(self.sample_rate)

    def listener_callback(self, msg):
        # msg.data is a bytes object containing interleaved PCM samples
        self.wav_file.writeframes(msg.data)

    def destroy_node(self):
        self.wav_file.close()
        super().destroy_node()

def main(args=None):
    rclpy.init(args=args)
    node = AudioSaver()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()

if __name__ == '__main__':
    main()
