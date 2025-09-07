#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from audio_common_msgs.msg import AudioData
import pyaudio
import wave
import struct
import time
import array

class MicListener(Node):
    def __init__(self):
        super().__init__('mic_listener')

        # ROS 2 parameter: 'play' or 'record'
        self.declare_parameter('mode', 'record')
        self.mode = self.get_parameter('mode').get_parameter_value().string_value

        # Audio format — must match Kinect mic node
        self.RATE = 16000       # Hz
        self.CHANNELS = 1       # mono (mixed down in mic node)
        self.FORMAT = pyaudio.paInt16
        self.FRAMES_PER_BUFFER = 4096

        self.p = pyaudio.PyAudio()

        if self.mode == 'play':
            self.stream = self.p.open(format=self.FORMAT,
                                      channels=self.CHANNELS,
                                      rate=self.RATE,
                                      output=True,
                                      frames_per_buffer=self.FRAMES_PER_BUFFER)
            self.get_logger().info("Mic listener started — live playback mode")
        else:
            # Avoid querying default device: use struct.calcsize for sample width
            self.wav_file = wave.open('mic_capture.wav', 'wb')
            self.wav_file.setnchannels(self.CHANNELS)
            self.wav_file.setsampwidth(struct.calcsize('h'))  # 2 bytes for int16
            self.wav_file.setframerate(self.RATE)
            self.get_logger().info("Mic listener started — recording to mic_capture.wav for 15 seconds")
            self.start_time = time.time()

        self.subscription = self.create_subscription(
            AudioData,
            '/kinect/audio',
            self.audio_callback,
            10
        )

    def audio_callback(self, msg):
        if self.mode == 'play':
            try:
                self.stream.write(bytes(msg.data))
            except IOError as e:
                self.get_logger().error(f"Playback error: {e}")
        else:
            # Convert bytes → signed 16‑bit array
            samples = array.array('h', msg.data)
            # Apply gain reduction (e.g., 0.5 = -6 dB)
            gain = 0.5
            for i in range(len(samples)):
                samples[i] = int(samples[i] * gain)
            self.wav_file.writeframes(bytes(msg.data))
            # Stop after 15 seconds
            if time.time() - self.start_time >= 15.0:
                self.get_logger().info("15 seconds reached — stopping recording")
                self.destroy_node()

    def destroy_node(self):
        if self.mode == 'play':
            self.stream.stop_stream()
            self.stream.close()
        else:
            self.wav_file.close()
        self.p.terminate()
        super().destroy_node()

def main(args=None):
    rclpy.init(args=args)
    node = MicListener()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        if rclpy.ok():
            node.destroy_node()
        rclpy.shutdown()

if __name__ == '__main__':
    main()
