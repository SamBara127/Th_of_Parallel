import argparse
import cv2
import time
from ultralytics import YOLO # type: ignore
import numpy as np
from concurrent.futures import ThreadPoolExecutor, ProcessPoolExecutor

class VideoProcessor:
    def __init__(self, video_path, mode, output_path):
        self.video_path = video_path
        self.mode = mode
        self.output_path = output_path
        self.model = YOLO('yolov8s-pose.pt')

    def process_frame(self, frame):
        results = self.model(frame)
        # print(results.__annotations__)
        # for result in results.__annotations__:
        #     bbox = result[0:4]  # координаты bounding box
        #     cv2.rectangle(frame, (int(bbox[0]), int(bbox[1])), (int(bbox[2]), int(bbox[3])), (0, 255, 0), 2)
        return frame

    def process_video_single_thread(self):
        cap = cv2.VideoCapture(self.video_path)
        width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
        height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
        fps = int(cap.get(cv2.CAP_PROP_FPS))
        # out = cv2.VideoWriter(self.output_path, cv2.VideoWriter_fourcc(*'XVID'), fps, (width, height))
        out = cv2.VideoWriter(self.output_path, cv2.VideoWriter_fourcc(*'mp4v'), fps, (width, height))

        start_time = time.time()
        while cap.isOpened():
            ret, frame = cap.read()
            print(frame.shape)
            if not ret:
                break
            # processed_frame = self.process_frame(frame)
            out.write(frame)
        # results = self.model(source="video.mp4", show=True, conf=0.3, save=False)
        end_time = time.time()

        cap.release()
        out.release()
        return end_time - start_time

    def process_video_multi_thread(self):
        cap = cv2.VideoCapture(self.video_path)
        width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
        height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
        fps = int(cap.get(cv2.CAP_PROP_FPS))
        out = cv2.VideoWriter(self.output_path, cv2.VideoWriter_fourcc(*'XVID'), fps, (width, height))

        frames = []
        while cap.isOpened():
            ret, frame = cap.read()
            if not ret:
                break
            frames.append(frame)

        cap.release()

        start_time = time.time()
        with ThreadPoolExecutor() as executor:
            processed_frames = list(executor.map(self.process_frame, frames))

        for frame in processed_frames:
            out.write(frame)
        end_time = time.time()

        out.release()
        return end_time - start_time

    def run(self):
        if self.mode == 'single':
            duration = self.process_video_single_thread()
        else:
            duration = self.process_video_multi_thread()
        print(f"Processing time: {duration:.2f} seconds")


def main():
    parser = argparse.ArgumentParser(description="Video processing with YOLOv8 pose model")
    parser.add_argument('video_path', type=str, help='Path to the input video')
    parser.add_argument('mode', type=str, choices=['single', 'multi'], help='Processing mode: single or multi thread')
    parser.add_argument('output_path', type=str, help='Path to the output video')
    args = parser.parse_args()

    processor = VideoProcessor(args.video_path, args.mode, args.output_path)
    processor.run()

if __name__ == "__main__":
    main()
