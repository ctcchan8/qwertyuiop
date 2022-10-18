from jen import *


def run(worker: WorkerController):
    worker.init()
    worker.use_clock(frequency=100)
    sm = worker.use_serial_manager()
    
    # Change this!
    sm.whitelist.append(PortInfo(serial_number="5513132373735171A0B1", baudrate=115200))
    sm.whitelist.append(PortInfo(serial_number="7513131383235170F071", baudrate=115200))

    gb.start_gateway(UDPBroadcast("255.255.255.255", 7986))

    gen_output = gb.clone("rg.o")
    carbease_output = gb.clone("rc.o")

    while True:
        if isBtnJustPressed(RIGHT_L):
            gen_output[0] = not gen_output[0]

        if isBtnJustPressed(RIGHT_D):
            gen_output[1] = not gen_output[1]

        gb.write("rg.o", list(gen_output))

        vx = getAxis(LEFT_X)
        vy = getAxis(LEFT_Y)
        A = getAxis(RIGHT_X)

        # Calculate Speeds
        vz = (vx + vy) * A
        speed_multi = 2000
        wheelspeed = [
            (+ vx - vy - vz) * ( + speed_multi),
            (- vx - vy + vz) * ( - speed_multi),
            (- vx + vy - vz) * ( + speed_multi),
            (+ vx + vy + vz) * ( - speed_multi)
        ]
        for i in range(4):
            carbease_output[i] = wheelspeed[i]

        gb.write("rc.o", list(carbease_output))

        ## print("local", gen_output)
        ## ## print("e", time.perf_counter())
        ## print("feedback", gb.read("rs.f"))

        worker.spin()
