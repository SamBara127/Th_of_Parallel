import cv2
import threading
import queue
import argparse
import logging
import time

# Базовый класс Sensor который является экземпляром для других подклассов которые будуь его наследовать
class Sensor:
    def __init__(self, frequency):
        # частота обновления - это общая характеристика
        self.frequency = frequency
        # буффер хранения данных считанные из датчика (к примеру в камере это фрейм изображение кадра)
        # запись происходит после вызова метода get() 
        self.data = None

    def get(self):
        # в случае если этот метод в подклассе не переопределен, то метод из родительского класса выбросит исключение
        # давая прямой намек на то что его необходимо иметь в дочернем классе
        raise NotImplementedError("Subclasses must implement method get()")
    

# Класс SensorCam
class SensorCam(Sensor):
    def __init__(self, cam_name=0, resolution=(640, 480)):
        # передача родительскому классу значение частоты в его конструктор
        super().__init__(frequency=30)  # Частота захвата кадров, например 30 fps
        self.cam_name = cam_name
        self.resolution = resolution
        self.cap = cv2.VideoCapture(cam_name)
        if not self.cap.isOpened():
            logging.error(f"Camera {cam_name} is not open.")
            raise Exception(f"Camera {cam_name} is not open.")
        self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, resolution[0])
        self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, resolution[1])

    def get(self):
        ret, frame = self.cap.read()
        if not ret:
            logging.error("Error of reading frame in camera.")
            raise Exception("Error of reading frame in camera.")
        self.data = frame

    def __del__(self):
        if self.cap.isOpened():
            self.cap.release()


# Класс SensorX
class SensorX(Sensor):
    def __init__(self, frequency):
        self.frequency = int((1/frequency))
        super().__init__(self.frequency)

    def get(self):
        # Симуляция получения данных с датчика
        self.data = f"Data from SensorX with frequency {self.frequency} Hz"


# Класс WindowImage
class WindowImage:
    def __init__(self, display_frequency):
        self.display_frequency = display_frequency

    def show(self, img):
        cv2.imshow('Sensor_Data', img)

    def __del__(self):
        cv2.destroyAllWindows()


def sensor_thread(sensor: SensorX, data_queue: queue.Queue, stop_event: threading.Event):
    while not stop_event.is_set():
        sensor.get()
        data_queue.put(sensor.data)
        time.sleep(1 / sensor.frequency)


def sensor_thread_cam(sensor: SensorCam, data_queue: queue.Queue, stop_event: threading.Event):
    while not stop_event.is_set():
        sensor.get()
        data_queue.put(sensor.data)
        # time.sleep(1 / sensor.frequency)


def main(cam_name, resolution, display_frequency):

    sensor_cam = SensorCam(cam_name, resolution)
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

    window = WindowImage(display_frequency)
    try:
        time.sleep(1)  # Даем потокам время для запуска и заполнения очередей
        while True:
            try:
                frame = cam_queue.get(timeout=1/display_frequency)
                try:
                    sensor_data = sensor0_queue.get_nowait()
                    print(sensor_data)
                    sensor_data = sensor1_queue.get_nowait()
                    print(sensor_data)
                    sensor_data = sensor2_queue.get_nowait()
                    print(sensor_data)
                except queue.Empty:
                    # print("Очередь пуста. Ожидание новых данных с датчиков...")
                    pass
                window.show(frame)
                if cv2.waitKey(1) & 0xFF == ord('q'):
                    break
            except queue.Empty:
                print("Очередь пуста. Ожидание нового кадра...")
                # time.sleep(0.1)  # Небольшая задержка перед повторной попыткой
    finally:
        del window
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