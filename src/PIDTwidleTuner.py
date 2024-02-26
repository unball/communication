from serialReader import SerialReader
from time import sleep

BUFFER_SIZE = 10
data = []

serial = SerialReader("/dev/ttyUSB0", 115200)

erro_teste = 0.0

def salva_constantes(parametros):
    try:
        os.remove(nome_arquivo)
    except FileNotFoundError:
        pass
    
    with open(Constantes, "w") as file:
        file.write(f"K {parametros.k}\n")
        file.write(f"DK {parametros.dk}\n")
        file.write(f"Ksi {parametros.ksi}\n")
        file.write(f"Erro {parametros.erro}\n")

def le_constantes(parametro):
    if os.path.exists("Constantes.dat"):
        with open("Constantes.dat", "r") as file:
            parametro.k = eval(file.readline().strip()[1:])
            parametro.dk = eval(file.readline().strip()[2:])
            parametro.ksi = float(file.readline().strip()[3:])
            parametro.erro = eval(file.readline().strip()[4:])
    return parametro

def run_test_PID(k):
    mensagem = str( "T" + parametros.k[0]) + " " + str(parametros.k[1]) + " " + str(parametros.k[2] + "\n")
    serial.write(mensagem.encode())
    sleep(16)
    erro = float(serial.read())
    print(mensagem)
    return erro


class Parametros:
    def __init__(self):
        self.k = [0.0, 0.0, 0.0]
        self.dk = [0.0, 0.0, 0.0]
        self.ksi = 0.0
        self.erro = 0.0

def twidle(parametro):
    parametro = le_constantes(parametro)
    for i in range(3):
        # Define um erro se não tivermos um erro
        if parametro.erro == 0.0:
            parametro.erro = run_test_PID(parametro.k)

        parametro.k[i] += parametro.dk[i]
        erro_teste = run_test_PID(parametro.k)

        if erro_teste < parametro.erro:
            parametro.erro = erro_teste
            parametro.k[i] *= 1 + parametro.ksi
        else:
            parametro.k[i] -= 2 * parametro.dk[i]
            erro_teste = run_test_PID(parametro.k)
            
            if erro_teste < parametro.erro:
                parametro.erro = erro_teste
                parametro.k[i] *= 1 + parametro.ksi
            else:
                parametro.k[i] += parametro.dk[i]
                parametro.dk[i] *= 1 - parametro.ksi
    salva_constantes
    return parametro

if __name__ == "__main__":
    # Recebe um buffer de dados
    while len(data) <= BUFFER_SIZE-1:
        values = serial.read()

        # Concatena ao conjunto de dados lidos
        if values is not None:
            data.append(values)
    parametros = Parametros()
    parametros = twidle(parametros)

