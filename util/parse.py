import argparse
import copy
import fileinput
import itertools
import re
from typing import List

from dataclasses import dataclass, field
from matplotlib import pyplot as plt


filters = {}

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

def movingAverage(readings: List, size = 5, init_value=0):
    output = []

    winidx = 0
    values = [init_value] * size
    acc = init_value * size
    for i in readings:
        acc -= values[winidx]
        acc += i
        values[winidx] = i
        winidx = (winidx+1) % size
        output.append(acc/size)
    return output

def movingVariance(readings: List, size = 5, init_value=0):
    output = []
    winidx = 0
    values = [init_value] * size
    squares = [init_value*init_value] * size
    
    acc =  init_value * size
    square_acc = init_value * init_value * size

    for i in readings:
        acc -= values[winidx]
        acc += i
        values[winidx] = i
        winidx = (winidx+1) % size

        square_acc -= squares[winidx]
        square_acc += i * i
        squares[winidx] = i * i
        winidx = (winidx+1) % size

        variance = (square_acc - (acc * acc )/size)/(size-1)
        output.append(variance)

    return output

def autocalibrateWithStandardDev(readings, inner_window=5, outer_window=500):
    output = []

    filtered = movingAverage(readings, inner_window)

    baseline = movingAverage(readings, outer_window, 5000) 

    variance = movingVariance(readings, outer_window, 0)

    for f, b, v in zip(filtered, baseline, variance):
        if f > b and (f - b)*(f - b) > v:
            output.append(f)
        else: 
            output.append(0)

    return output

filters['movingwindow'] = movingAverage
filters['movingwindowlong'] = lambda readings: movingAverage(readings, 500)
filters['movingwindow2pass'] = lambda readings: movingAverage(movingwindowfilter(readings))


movingwindowautocalibrate = lambda readings: [max(value - calibration, 0) for value,calibration in zip(movingAverage(readings, size=5),movingAverage(readings, size=1000, init_value=5000)) ]
filters['movingwindowautocalibrate']  = movingwindowautocalibrate


quantizevalues = itertools.cycle(range(500,500*6,500))
quantize = lambda function: lambda readings: (lambda readings, value: [value if i else 0 for i in function(readings)])(readings, next(quantizevalues))
filters['qtest'] = quantize(lambda readings: [1 for i in readings])


filters['movingwindowautocalibratequantize'] =  quantize(movingwindowautocalibrate)

filters['autocalstddev'] = autocalibrateWithStandardDev
filters['autocalstddevquantize'] = quantize(autocalibrateWithStandardDev)
filters['showvariance'] = movingVariance

def dofilter(data, filter_fn):
    newdata = copy.deepcopy(data)
    newdata.analog_readings = [filter_fn(channel) for channel in data.analog_readings]
    return newdata


parser = argparse.ArgumentParser()
parser.add_argument('filepath',)
parser.add_argument('--encoding', default='utf-8')
parser.add_argument('--filter', default=None, help="Filter function to run. Does NOT recompute transmission events.")

if __name__ == "__main__":
    args = parser.parse_args()
    data = read(args.filepath, args.encoding)
    if args.filter:
        data = dofilter(data, filters[args.filter])
    show(data)
