"""P04: publish K230 face detections as PanView terminal text.

This script is intentionally independent from the vendor example. It reuses the
same face-detection model pipeline but only publishes to the CanMV IDE terminal.
UART publishes the same diagnostic text; the P08 binary protocol remains out of scope.
"""


# These layouts are documented by different CanMV K230 package generations.
# A model and its matching anchor file must exist in the same layout.
K230_RESOURCE_LAYOUTS = (
    (
        "/sdcard/examples/kmodel/face_detection_320.kmodel",
        "/sdcard/examples/utils/prior_data_320.bin",
    ),
    (
        "/sdcard/kmodel/face_detection_320.kmodel",
        "/sdcard/utils/prior_data_320.bin",
    ),
    (
        "/sdcard/app/tests/kmodel/face_detection_320.kmodel",
        "/sdcard/app/tests/utils/prior_data_320.bin",
    ),
)

# Unit: bit/s. Source: vendor `02.Basic/05.uart.py`; applies to P04 text output.
K230_UART_BAUDRATE = 115200


def get_display_config(display_mode):
    """Return a supported display mode and size for the current CanMV board."""
    if display_mode == "lcd":
        # The Yahboom firmware reports an ST7701 LCD; its documented panel size is 800x480.
        return "lcd", [800, 480]
    if display_mode == "hdmi":
        return "hdmi", [1920, 1080]
    raise ValueError("unsupported display mode: {}".format(display_mode))


def resolve_resource_paths(path_exists):
    """Return the first complete model/anchor pair visible on the device."""
    checked_layouts = []
    for model_path, anchors_path in K230_RESOURCE_LAYOUTS:
        checked_layouts.append("{} + {}".format(model_path, anchors_path))
        if path_exists(model_path) and path_exists(anchors_path):
            return model_path, anchors_path
    raise OSError(
        "K230 face resources not found; checked: {}".format("; ".join(checked_layouts))
    )


def _pixel_box(detection):
    """Return the first four detection values as source-image integer pixels."""
    if len(detection) < 4:
        raise ValueError("detection must contain x, y, w and h")
    return tuple(int(round(float(value))) for value in detection[:4])


def choose_primary_detection(detections):
    """Choose the largest valid detection box as the single PanView target."""
    if not detections:
        return None
    return max((_pixel_box(detection) for detection in detections), key=lambda box: box[2] * box[3])


def format_result_line(detections, timestamp_ms, fps):
    """Format one stable, human-readable P04 result line."""
    count = len(detections)
    box = choose_primary_detection(detections)
    if box is None:
        return "PV04 t_ms={} target=0 count=0 confidence=unavailable fps={:.1f}".format(timestamp_ms, fps)

    x, y, width, height = box
    center_x = x + width / 2.0
    center_y = y + height / 2.0
    return (
        "PV04 t_ms={} target=1 count={} x={} y={} w={} h={} cx={:.1f} cy={:.1f} "
        "confidence=unavailable fps={:.1f}"
    ).format(timestamp_ms, count, x, y, width, height, center_x, center_y, fps)


def format_uart_payload(result_line):
    """Terminate one P04 text result for a line-oriented serial receiver."""
    return "{}\r\n".format(result_line)


def send_uart_payload(uart, payload):
    """Write one already formatted text payload through a machine.UART object."""
    return uart.write(payload.encode("ascii"))


def create_k230_uart(fpioa_factory, uart_type):
    """Map 12Pin IO32 to UART3 TX and return the configured transmitter."""
    fpioa = fpioa_factory()
    fpioa.set_function(32, fpioa.UART3_TXD, ie=0, oe=1)
    fpioa.set_function(33, fpioa.UART3_RXD, ie=1, oe=0)
    return uart_type(uart_type.UART3, baudrate=K230_UART_BAUDRATE)


# Keeping vendor imports behind this guard lets the pure functions above be
# tested on the PC without pretending that the PC has a K230 runtime.
CANMV_RUNTIME = True
CANMV_IMPORT_ERROR = None
try:
    from libs.PipeLine import PipeLine, ScopedTiming
    from libs.AIBase import AIBase
    from libs.AI2D import Ai2d
    from machine import FPIOA, UART
    from media.media import ALIGN_UP
    import aidemo
    import gc
    import nncase_runtime as nn
    import os
    import sys
    import time
    import ulab.numpy as np
except ImportError as error:
    CANMV_RUNTIME = False
    CANMV_IMPORT_ERROR = error


if CANMV_RUNTIME:
    class FaceDetectionApp(AIBase):
        """Vendor face-detection flow kept separate from PanView publishing."""

        def __init__(self, kmodel_path, model_input_size, anchors,
                     confidence_threshold=0.5, nms_threshold=0.2,
                     rgb888p_size=[1920, 1080], display_size=[1920, 1080], debug_mode=0):
            super().__init__(kmodel_path, model_input_size, rgb888p_size, debug_mode)
            self.model_input_size = model_input_size
            self.confidence_threshold = confidence_threshold
            self.nms_threshold = nms_threshold
            self.anchors = anchors
            self.rgb888p_size = [ALIGN_UP(rgb888p_size[0], 16), rgb888p_size[1]]
            self.display_size = [ALIGN_UP(display_size[0], 16), display_size[1]]
            self.debug_mode = debug_mode
            self.ai2d = Ai2d(debug_mode)
            self.ai2d.set_ai2d_dtype(
                nn.ai2d_format.NCHW_FMT,
                nn.ai2d_format.NCHW_FMT,
                np.uint8,
                np.uint8,
            )

        def config_preprocess(self, input_image_size=None):
            with ScopedTiming("set preprocess config", self.debug_mode > 0):
                ai2d_input_size = input_image_size if input_image_size else self.rgb888p_size
                top, bottom, left, right = self.get_padding_param()
                self.ai2d.pad([0, 0, 0, 0, top, bottom, left, right], 0, [104, 117, 123])
                self.ai2d.resize(nn.interp_method.tf_bilinear, nn.interp_mode.half_pixel)
                self.ai2d.build(
                    [1, 3, ai2d_input_size[1], ai2d_input_size[0]],
                    [1, 3, self.model_input_size[1], self.model_input_size[0]],
                )

        def postprocess(self, results):
            with ScopedTiming("postprocess", self.debug_mode > 0):
                post_ret = aidemo.face_det_post_process(
                    self.confidence_threshold,
                    self.nms_threshold,
                    self.model_input_size[1],
                    self.anchors,
                    self.rgb888p_size,
                    results,
                )
                return post_ret[0] if post_ret else post_ret

        def draw_result(self, pipeline, detections):
            with ScopedTiming("display_draw", self.debug_mode > 0):
                pipeline.osd_img.clear()
                for detection in detections:
                    x, y, width, height = _pixel_box(detection)
                    x = x * self.display_size[0] // self.rgb888p_size[0]
                    y = y * self.display_size[1] // self.rgb888p_size[1]
                    width = width * self.display_size[0] // self.rgb888p_size[0]
                    height = height * self.display_size[1] // self.rgb888p_size[1]
                    pipeline.osd_img.draw_rectangle(
                        x, y, width, height, color=(255, 255, 0, 255), thickness=2
                    )

        def get_padding_param(self):
            dst_width, dst_height = self.model_input_size
            ratio = min(dst_width / self.rgb888p_size[0], dst_height / self.rgb888p_size[1])
            new_width = int(ratio * self.rgb888p_size[0])
            new_height = int(ratio * self.rgb888p_size[1])
            top = 0
            bottom = int(round((dst_height - new_height) + 0.1))
            left = 0
            right = int(round((dst_width - new_width) - 0.1))
            return top, bottom, left, right


    def run_publisher():
        display_mode, display_size = get_display_config("lcd")
        rgb888p_size = [1920, 1080]

        def path_exists(path):
            try:
                os.stat(path)
                return True
            except OSError:
                return False

        kmodel_path, anchors_path = resolve_resource_paths(path_exists)
        anchors = np.fromfile(anchors_path, dtype=np.float).reshape((4200, 4))
        pipeline = PipeLine(
            rgb888p_size=rgb888p_size,
            display_size=display_size,
            display_mode=display_mode,
        )
        pipeline.create()
        face_detection = FaceDetectionApp(
            kmodel_path,
            model_input_size=[320, 320],
            anchors=anchors,
            confidence_threshold=0.5,
            nms_threshold=0.2,
            rgb888p_size=rgb888p_size,
            display_size=display_size,
        )
        face_detection.config_preprocess()
        previous_ms = None
        raw_dump_remaining = 5
        nonempty_raw_dumped = False
        uart = None
        print(
            "PV04 start source=face_detection image=1920x1080 display={}x{} confidence=unavailable".format(
                display_size[0], display_size[1]
            )
        )

        try:
            uart = create_k230_uart(FPIOA, UART)
            while True:
                os.exitpoint()
                with ScopedTiming("total", 1):
                    image = pipeline.get_frame()
                    detections = face_detection.run(image)
                    now_ms = time.ticks_ms()
                    if previous_ms is None:
                        fps = 0.0
                    else:
                        elapsed_ms = time.ticks_diff(now_ms, previous_ms)
                        fps = 1000.0 / elapsed_ms if elapsed_ms > 0 else 0.0
                    previous_ms = now_ms

                    face_detection.draw_result(pipeline, detections)
                    pipeline.show_image()
                    if raw_dump_remaining > 0:
                        print("PV04 raw_dets={}".format(detections))
                        raw_dump_remaining -= 1
                    if detections and not nonempty_raw_dumped:
                        print("PV04 raw_dets_nonempty={}".format(detections))
                        nonempty_raw_dumped = True
                    result_line = format_result_line(detections, now_ms, fps)
                    print(result_line)
                    send_uart_payload(uart, format_uart_payload(result_line))
                    gc.collect()
        except Exception as error:
            sys.print_exception(error)
        finally:
            if uart is not None:
                uart.deinit()
            face_detection.deinit()
            pipeline.destroy()


if __name__ == "__main__":
    if not CANMV_RUNTIME:
        raise RuntimeError("This script must run on CanMV K230: {}".format(CANMV_IMPORT_ERROR))
    run_publisher()
