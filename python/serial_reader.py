import serial
import matplotlib.pyplot as plt
from serial.tools import list_ports
from time import time, sleep

def interp(a,b):
    return (a+b)/2

def interp_array(array):
    tmp = []
    post_interp = []

    i = 0
    j = 0
    while True:
        if i >= len(array[0])-1:
            tmp.append(array[j][i])
            post_interp.append(tmp)
            tmp = []

            i = 0
            j += 1
            if j >= len(array):
                break
            
        tmp.append(array[j][i])
        tmp.append(interp(array[j][i], array[j][i+1]))
        i += 1
    return post_interp


def interp_array2d(array):

    array1d = interp_array(array)

    tmp = []
    post_interp = []

    i = 0
    j = 0
    while True:
        if i >= len(array1d)-1:
            tmp.append(array1d[i][j])
            post_interp.append(tmp)
            tmp = []
            i = 0
            j += 1
            if j >= len(array1d[0]):
                break
        tmp.append(array1d[i][j])
        tmp.append(interp(array1d[i][j], array1d[i+1][j]))
        i += 1
    return [list(i) for i in zip(*post_interp)]

# Get a list of available serial ports
ports = list_ports.comports()

# Print the list of ports
for port in ports:
    print(port.device)

# Open the serial port
ser = serial.Serial('COM4', 9600)

# Create a figure and axis for the plot
fig, ax = plt.subplots()

# Initialize the plot with empty data
data_plot = ax.imshow([[],[]], cmap='jet',interpolation='nearest')

# Add colorbar
cbar = plt.colorbar(data_plot)

# Set the number of columns and rows
num_columns = 32
num_rows = 24

offsets = []
for i in range(num_columns):
    for j in range(num_rows):
        offsets.append((i, j))

# Read serial output and update the plot
while True:
    data = ser.readline().decode().strip()
    data = data.split(',')[:-1]
    data = [float(d) for d in data]  # Convert numbers to floats

    # Check if data size is correct
    if len(data) != num_rows * num_columns:
        print(f"Expected {num_rows * num_columns} data points, got {len(data)}")
        continue

    # Reshape the data into a 2D array
    data = [data[i:i + num_columns] for i in range(0, len(data), num_columns)]

    # Interpolate the data
    interpolate_steps = 2
    interp_data = data
    for i in range(interpolate_steps):
        interp_data = interp_array2d(interp_data)


    # data_plot.set_offsets(offsets)
    ax.imshow(interp_data, cmap='jet', interpolation='none')
    # Set the plot limits
    ax.set_xlim(-1, len(interp_data[0]))
    ax.set_ylim(-1, len(interp_data))

    # fig.canvas.draw_idle()

    # # Update the plot
    plt.pause(0.01)

    plt.cla()

# Close the serial port
ser.close()
