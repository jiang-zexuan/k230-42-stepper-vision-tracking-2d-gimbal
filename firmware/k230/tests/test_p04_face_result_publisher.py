import pathlib
import sys
import unittest


APP_DIR = pathlib.Path(__file__).resolve().parents[1] / "app"
sys.path.insert(0, str(APP_DIR))

from p04_face_result_publisher import (
    choose_primary_detection,
    create_k230_uart,
    send_uart_payload,
    format_uart_payload,
    format_result_line,
    get_display_config,
    resolve_resource_paths,
)


class FaceResultPublisherTests(unittest.TestCase):
    def test_no_detection_publishes_explicit_target_absent_state(self):
        line = format_result_line([], timestamp_ms=42, fps=0.0)

        self.assertEqual("PV04 t_ms=42 target=0 count=0 confidence=unavailable fps=0.0", line)

    def test_largest_detection_becomes_primary_target(self):
        detections = [
            [1.2, 2.8, 3.1, 4.9],
            [10.0, 20.0, 20.0, 5.0],
        ]

        self.assertEqual((10, 20, 20, 5), choose_primary_detection(detections))
        self.assertEqual(
            "PV04 t_ms=42 target=1 count=2 x=10 y=20 w=20 h=5 cx=20.0 cy=22.5 confidence=unavailable fps=12.5",
            format_result_line(detections, timestamp_ms=42, fps=12.5),
        )

    def test_resource_resolver_uses_available_k230_layout(self):
        available = {
            "/sdcard/examples/kmodel/face_detection_320.kmodel",
            "/sdcard/examples/utils/prior_data_320.bin",
        }

        self.assertEqual(
            (
                "/sdcard/examples/kmodel/face_detection_320.kmodel",
                "/sdcard/examples/utils/prior_data_320.bin",
            ),
            resolve_resource_paths(available.__contains__),
        )

    def test_resource_resolver_reports_all_known_layouts_when_missing(self):
        with self.assertRaises(OSError) as context:
            resolve_resource_paths(lambda _path: False)

        self.assertIn("/sdcard/examples/kmodel/face_detection_320.kmodel", str(context.exception))
        self.assertIn("/sdcard/kmodel/face_detection_320.kmodel", str(context.exception))
        self.assertIn("/sdcard/app/tests/kmodel/face_detection_320.kmodel", str(context.exception))

    def test_yahboom_st7701_uses_lcd_resolution(self):
        self.assertEqual(("lcd", [800, 480]), get_display_config("lcd"))

    def test_uart_payload_uses_crlf_as_the_text_frame_boundary(self):
        self.assertEqual(
            "PV04 t_ms=42 target=0\r\n",
            format_uart_payload("PV04 t_ms=42 target=0"),
        )

    def test_uart_payload_uses_the_machine_uart_write_api(self):
        class FakeUart:
            def __init__(self):
                self.payloads = []

            def write(self, payload):
                self.payloads.append(payload)
                return len(payload)

        uart = FakeUart()

        self.assertEqual(23, send_uart_payload(uart, "PV04 t_ms=42 target=0\r\n"))
        self.assertEqual([b"PV04 t_ms=42 target=0\r\n"], uart.payloads)

    def test_uart3_output_is_mapped_to_12pin_io32(self):
        class FakeFpioa:
            UART3_RXD = "uart3_rxd"
            UART3_TXD = "uart3_txd"

            def __init__(self):
                self.calls = []

            def set_function(self, pin, function, **options):
                self.calls.append((pin, function, options))

        class FakeUart:
            UART3 = 3

            def __init__(self, port, baudrate):
                self.port = port
                self.baudrate = baudrate

        fpioa_instances = []

        def create_fpioa():
            instance = FakeFpioa()
            fpioa_instances.append(instance)
            return instance

        uart = create_k230_uart(create_fpioa, FakeUart)

        self.assertEqual(
            [
                (32, FakeFpioa.UART3_TXD, {"ie": 0, "oe": 1}),
                (33, FakeFpioa.UART3_RXD, {"ie": 1, "oe": 0}),
            ],
            fpioa_instances[0].calls,
        )
        self.assertEqual(FakeUart.UART3, uart.port)
        self.assertEqual(115200, uart.baudrate)


if __name__ == "__main__":
    unittest.main()
