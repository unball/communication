import serial
from time import sleep
from random import randint
import os

class Twiddle():
    def __init__(self):
        self.id = -1
        self.serial = None
        self.failCount = 0

    def send(self, kp, ki, kd):
        try:
            if self.serial is None:
                self.serial = serial.Serial('/dev/ttyUSB0', 115200)
                self.serial.timeout = 0.100
        except Exception as e:
            print(e)
            print("Falha ao abrir serial")
            return
        
        # Início da mensagem
        message = bytes("TTT", encoding='ascii')

        # Robot's ID
        message += (self.id).to_bytes(2,byteorder='little', signed=True)

        # Control parameters
        message += (int(kp)).to_bytes(2,byteorder='little', signed=True)
        message += (int(ki)).to_bytes(2,byteorder='little', signed=True)
        message += (int(kd)).to_bytes(2,byteorder='little', signed=True)

        try:
            self.serial.write(message)
        except Exception as e:
            self.failCount += 1
            print("Falha ao enviar: " + str(self.failCount) + ", " + str(e))

            if self.failCount >= 30:
                self.serial.close()
                self.serial = None
                self.failCount = 0

    def receive(self):
        response = self.serial.readline().decode()
        try:
            if len(response) != 1:
                print('Mensagem de erro de tamanho errado')
                return 0
            else: return response
        except:
            print('Sem resposta de erro')
            return 0

    def closeSerial(self):
        if self.serial is not None: self.serial.close()      

    def twiddle(self, k, dk, ksi=.3, target=None):
        if not target:
            target = self.run_pid_test(*k)

        for i, _ in enumerate(k):
            k[i] += dk[i]
            new_error = self.run_pid_test(*k)

            if new_error < target:
                target = new_error
                dk[i] *= 1 + ksi

            else:
                k[i] -= 2*dk[i]
                new_error = self.run_pid_test(*k)

                if new_error < target:
                    target = new_error
                    dk[i] *= 1 + ksi

                else:
                    k[i] += dk[i]
                    dk[i] *= 1 - ksi

        self.closeSerial()

        return k, dk, ksi, target

    def run_pid_test(self, kp, ki, kd):
        print('kp: ', kp, '\nki: ', ki, '\nkd: ', kd)
        self.send(100*kp, 100*ki, 100*kd)
        sleep(8)
        error = self.receive()
        print('erro: ', error)
        return abs(float(error))
    
    def save_constants(params):
        try:
            os.remove('Constantes')
        except FileNotFoundError:
            pass

        with open('Constantes', 'w') as file:
            file.write(f"best (kp, ki kd): {params[0]}\n")
            file.write(f"best error {params[3]}\n")
            file.write(f"current (dks):  {params[1]}\n")


params = [[-1.858962031647976, -0.16864975434220458, 0.16686768215869935], [0.538265, 0.049981750000000005, 0.049981750000000005]]

if __name__ == "__main__":
    twiddle = Twiddle()
    
    while True:
        id = input("(0) Robot 0\n"
                    "(1) Robot 1\n"
                    "(2) Robot 2\n").strip()
        
        twiddle.id = int(id)
        print('Twiddle for robot ', twiddle.id)
        
        command = input("(1) run one\n"
                        "(2) run n\n"
                        "(3) close\n").strip()

        if command == "3":
            break

        n = 1
        if command == "2":
            n = int(input("\tn: ").strip())

        for _ in range(n):
            params = twiddle.twiddle(*params)
            print("-------------------------------------------------------------------------------",
                "best (kp, ki, kd):", params[0],
                "\nbest error:", params[3],
                "\ncurrent (dks): ", params[1])
            twiddle.save_constants(params)

