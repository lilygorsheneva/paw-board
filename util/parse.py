import argparse
import fileinput
import re
from typing import List

from dataclasses import dataclass, field
from matplotlib import pyplot as plt

sensor_data_pattern = re.compile(r'.*\((\d+)\) SENSOR: SENSORLOG \| *(\d*) \| *(\d*) \| *(\d*) \| *(\d*) \| *(\d*) \|.*')
input_accept_pattern = re.compile(r'.*\((\d+)\) ENCODING: \| (.) \|.*')
calibration_pattern = re.compile(r'.*\((\d+)\) SENSOR: Thresholds \| *(\d*) \| *(\d*) \| *(\d*) \| *(\d*) \| *(\d*) \|.*')

@dataclass
class LogDump:
    filename: str
    analog_timestamps: List[int] =  field(default_factory=list)
    analog_readings: List[List[int]] = field(default_factory=list)
    
    calibration_timestamps: List[int] =  field(default_factory=list)
    calibration_readings: List[List[int]] = field(default_factory=list)

    tx_events: List[tuple[int, str]]= field(default_factory=list)

def read(filename, encoding):
    data = LogDump(filename=filename)
    for i in range(5): 
        data.analog_readings.append([])
        data.calibration_readings.append([])

    with fileinput.input(files=filename, encoding=encoding) as sensorlog:
        for line in sensorlog:
            sensor_data = sensor_data_pattern.match(line)
            if sensor_data:
                data.analog_timestamps.append(int(sensor_data.group(1)))
                for i in range(5):
                    data.analog_readings[i].append(int((sensor_data.group(i+2))))
            
            calibration_data = calibration_pattern.match(line)
            if calibration_data:
                data.calibration_timestamps.append(int(calibration_data.group(1)))
                for i in range(5):
                    data.calibration_readings[i].append(int((calibration_data.group(i+2))))
                    
            tx_data = input_accept_pattern.match(line)
            if tx_data:
                char = tx_data.group(2)
                if char == chr(127): char = 'del'
                data.tx_events.append((  int(tx_data.group(1)), char,  ))

        

    return data

def showanalogchannel(data, i, **kwargs):
    plt.plot(data.analog_timestamps, data.analog_readings[i], color = finger_color_map[i], **kwargs)

def showtxevents(data, **kwargs):
    plt.xticks([i[0] for i in data.tx_events],   labels= [i[1] for i in data.tx_events], **kwargs)

finger_color_map = ['blue', 'red', 'green', 'black', 'yellow']

def showcalibrationevents(data, i, **kwargs):
    plt.scatter(data.calibration_timestamps, data.calibration_readings[i], color = finger_color_map[i], marker="_", **kwargs)


def show(data):
    for i in range(5): 
        showanalogchannel(data, i)
        showcalibrationevents(data, i)
    showtxevents(data)
    plt.show()

parser = argparse.ArgumentParser()
parser.add_argument('filepath')
parser.add_argument('--encoding', default='utf-16')

if __name__ == "__main__":
    args = parser.parse_args()
    show(read(args.filepath, args.encoding))
