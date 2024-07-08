import cv2
import threading
import queue
import argparse
import logging
import time

# Базовый класс Sensor который является экземпляром для других подклассов которые будуь его наследовать
class Sensor:
    def get(self):
        # в случае если этот метод в подклассе не переопределен, то метод из родительского класса выбросит исключение
        # давая прямой намек на то что его необходимо иметь в дочернем классе
        raise NotImplementedError("Subclasses must implement method get()")
    

# Класс SensorCam
class SensorCam(Sensor):
    def __init__(self, cam_name=0, resolution=(640, 480), frequency=30):
        # передача родительскому классу значение частоты в его конструктор
        self._frequency = frequency  # Частота захвата кадров, например 30 fps
        self._cam_name = cam_name
        self._resolution = resolution
        self._cap = cv2.VideoCapture(cam_name)
        if not self._cap.isOpened():
            logging.error(f"Camera {cam_name} is not open.")
            raise Exception(f"Camera {cam_name} is not open.")
        self._cap.set(cv2.CAP_PROP_FRAME_WIDTH, resolution[0])
        self._cap.set(cv2.CAP_PROP_FRAME_HEIGHT, resolution[1])

    def get(self):
        time.sleep(1 / self._frequency)
        ret, frame = self._cap.read()
        if not ret:
            logging.error("Error of reading frame in camera.")
            raise Exception("Error of reading frame in camera.")
        return frame

    def __del__(self):
        if self._cap.isOpened():
            self._cap.release()


# Класс SensorX
class SensorX(Sensor):
    def __init__(self, delay: float):
        self._delay = delay
        self._data = 0

    def get(self) -> int:
        # Симуляция получения данных с датчика
        time.sleep(self._delay)
        self._data += 1
        return self._data


def sensor_thread(sensor: SensorX, data_queue: queue.Queue, stop_event: threading.Event):
    while not stop_event.is_set():
        data_queue.put(sensor.get())


def sensor_thread_cam(sensor: SensorCam, data_queue: queue.Queue, stop_event: threading.Event):
    while not stop_event.is_set():
        data_queue.put(sensor.get())


def main(cam_name, resolution, display_frequency):

    sensor_cam = SensorCam(cam_name, resolution, display_frequency)
    sensor0 = SensorX(0.01)
    sensor1 = SensorX(0.1)
    sensor2 = SensorX(1)

    cam_queue = queue.Queue()
    sensor0_queue = queue.Queue()
    sensor1_queue = queue.Queue()
    sensor2_queue = queue.Queue()

    stop_event = threading.Event()

    cam_thread = threading.Thread(target=sensor_thread_cam, args=(sensor_cam, cam_queue, stop_event))
    sensor0_thread = threading.Thread(target=sensor_thread, args=(sensor0, sensor0_queue, stop_event))
    sensor1_thread = threading.Thread(target=sensor_thread, args=(sensor1, sensor1_queue, stop_event))
    sensor2_thread = threading.Thread(target=sensor_thread, args=(sensor2, sensor2_queue, stop_event))

    cam_thread.start()
    sensor0_thread.start()
    sensor1_thread.start()
    sensor2_thread.start()

    try:
        while True: 
            try:
                # timeout используется в методе get очереди для того, чтобы ограничить время ожидания элемента из очереди
                # 1/display_frequency - секунд в данном случае
                frame = cam_queue.get(timeout=1/display_frequency)
                try:
                    sensor0_data = sensor0_queue.get_nowait()
                    # print(f"Sensor1: {sensor0_data}")
                    sensor1_data = sensor1_queue.get_nowait()
                    # print(f"Sensor2: {sensor1_data}")
                    sensor2_data = sensor2_queue.get_nowait()
                    # print(f"Sensor3: {sensor2_data}")
                except queue.Empty:
                    # print("Очередь пуста. Ожидание новых данных с датчиков...")
                    pass

                start_x, start_y = frame.shape[1] - 200, frame.shape[0] - 150
                font = cv2.FONT_HERSHEY_SIMPLEX
                font_scale = 1
                color = (0, 0, 0)  # Цвет текста - черный
                thickness = 2
                text_margin = 10

                box_width = 200
                box_height = 150
                cv2.rectangle(frame, (start_x, start_y + box_height), (start_x + box_width, start_y), (255, 255, 255), -1)

                cv2.putText(frame, f"Sensor1: {sensor0_data}", (start_x + text_margin-5, start_y + 90+30), font, font_scale, color, thickness)
                cv2.putText(frame, f"Sensor2: {sensor1_data}", (start_x + text_margin-5, start_y + 50+30), font, font_scale, color, thickness)
                cv2.putText(frame, f"Sensor3: {sensor2_data}", (start_x + text_margin-5, start_y + 10+30), font, font_scale, color, thickness)


                cv2.imshow('Sensor_Data', frame)
                if cv2.waitKey(1) & 0xFF == ord('q'):
                    break
            except queue.Empty:
                # print("Очередь пуста. Ожидание нового кадра...")
                pass
    finally:
        cv2.destroyAllWindows()
        stop_event.set()
        cam_thread.join()
        sensor0_thread.join()
        sensor1_thread.join()
        sensor2_thread.join()


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--cam_name', type=int, default=0, help='Имя камеры в системе, по умолчанию 0 для встроенной камеры')
    parser.add_argument('--resolution', type=str, default='640x480', help='Желаемое разрешение камеры, по умолчанию 640x480')
    parser.add_argument('--display_frequency', type=int, default=30, help='Частота отображения картинки, по умолчанию 30 fps')
    args = parser.parse_args()
    
    resolution = tuple(map(int, args.resolution.split('x')))

    main(args.cam_name, resolution, args.display_frequency)