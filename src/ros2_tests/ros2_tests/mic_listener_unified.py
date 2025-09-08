#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from audio_common_msgs.msg import AudioData
import pyaudio
import wave
import struct
import time
import array

class MicListenerUnified(Node):
    def __init__(self):
        super().__init__('mic_listener_unified')

        # Parameter: 'play' or 'record'
        self.declare_parameter('mode', 'play')
        self.mode = self.get_parameter('mode').get_parameter_value().string_value

        # Kinect v1 mic array: 4 channels, 16-bit PCM, 16 kHz
        self.RATE = 16000
        self.KINECT_CHANNELS = 4
        self.CHANNELS = 1  # output mono
        self.FORMAT = pyaudio.paInt16
        self.FRAMES_PER_BUFFER = 4096

        self.p = pyaudio.PyAudio()

        output_index = None
        if self.mode == 'play':
            output_index = self.get_preferred_output_device_index()

        if self.mode == 'play':
            self.stream = self.p.open(
                format=self.FORMAT,
                channels=self.CHANNELS,
                rate=self.RATE,
                output=True,
                output_device_index=output_index,
                frames_per_buffer=self.FRAMES_PER_BUFFER
            )
            self.get_logger().info("Mic listener (unified) started — live playback mode")
        else:
            self.wav_file = wave.open('mic_capture_unified.wav', 'wb')
            self.wav_file.setnchannels(self.CHANNELS)
            self.wav_file.setsampwidth(struct.calcsize('h'))
            self.wav_file.setframerate(self.RATE)
            self.get_logger().info("Mic listener (unified) started — recording to mic_capture_unified.wav for 15 seconds")
            self.start_time = time.time()

        # Subscribe to Kinect mic topic
        self.subscription = self.create_subscription(
            AudioData,
            '/kinect/audio',
            self.audio_callback,
            10
        )

    def get_preferred_output_device_index(self):
        """Try PulseAudio/PipeWire first, else fallback to default ALSA device."""
        pulse_candidates = []
        default_candidate = None

        for i in range(self.p.get_device_count()):
            dev = self.p.get_device_info_by_index(i)
            if dev['maxOutputChannels'] > 0:
                name_lower = dev['name'].lower()
                if 'pulse' in name_lower or 'pipewire' in name_lower:
                    pulse_candidates.append(i)
                if default_candidate is None:
                    default_candidate = i

        if pulse_candidates:
            self.get_logger().info(f"Using Pulse/PipeWire output device index {pulse_candidates[0]}")
            return pulse_candidates[0]

        if default_candidate is not None:
            self.get_logger().info(f"Using default ALSA output device index {default_candidate}")
            return default_candidate

        raise RuntimeError("No output device found")

    def audio_callback(self, msg):
        # Convert raw bytes to int16 array
        samples = array.array('h', msg.data)

        # Downmix 4 channels -> mono by averaging
        mono_samples = array.array('h')
        for i in range(0, len(samples), self.KINECT_CHANNELS):
            ch_sum = 0
            for ch in range(self.KINECT_CHANNELS):
                ch_sum += samples[i + ch]
            mono_samples.append(int(ch_sum / self.KINECT_CHANNELS))

        if self.mode == 'play':
            try:
                self.stream.write(mono_samples.tobytes())
            except IOError as e:
                self.get_logger().error(f"Playback error: {e}")
        else:
            self.wav_file.writeframes(mono_samples.tobytes())
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
        return super().destroy_node()

def main(args=None):
    rclpy.init(args=args)
    node = MicListenerUnified()
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
