import arduino_bridge
import time

def update():
	#print("update")
	salsa.digitalWrite(14,dl) # in3, dir righ
	salsa.setPWM(0,pl) # en3, en right 
	salsa.digitalWrite(13,dr) # in1 dir left
	salsa.setPWM(9,pr) # en1, left
	#print("done")

salsa = arduino_bridge.connection()
salsa.setup_ws2812_unique_color_output(6,5)
salsa.setup_pwm_output(0) # right pwm
salsa.setup_pwm_output(9) # left pwm 
salsa.setup_pwm_freq(0,15)
salsa.setup_pwm_freq(9,15)
salsa.setup_digital_output(13) #left
salsa.setup_digital_output(14) #right
print("salsa done")

dl = 1
pl = 0
dr = 1
pr = 50
update()
time.sleep(1)

dl = 0
pl = 50
dr = 1
pr = 50
update()
time.sleep(1)

dl = 0
pl = 50
dr = 0
pr = 50
update()
time.sleep(1)

dl = 1
pl = 50
dr = 0
pr = 50
update()
time.sleep(1)

dl = 1
pl = 0
dr = 1
pr = 0
update()
