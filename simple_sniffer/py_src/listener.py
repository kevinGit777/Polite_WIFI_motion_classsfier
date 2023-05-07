import serial, time 

ser = serial.Serial('COM7', 460800)

while True:
    print(ser.readline())
    #print("sleeping.......")
    #time.sleep(1)