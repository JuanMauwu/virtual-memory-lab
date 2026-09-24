# Genera los programas de prueba 2 y 3 (el 1 es el ejemplo del enunciado).
import random
PAGE = 4096
# Test 2: recorrido secuencial de 100 páginas, 2 pasadas (no cabe en 64 marcos)
with open("tests/test2_secuencial.txt", "w") as f:
    f.write("alloc %d\n" % (100 * PAGE))
    for _ in range(2):
        for p in range(100):
            f.write("read %d\n" % (p * PAGE))
# Test 3: localidad. 80% de accesos a 20 páginas "calientes", 20% al resto (100 páginas)
random.seed(42)
with open("tests/test3_localidad.txt", "w") as f:
    f.write("alloc %d\n" % (100 * PAGE))
    for _ in range(2000):
        p = random.randrange(20) if random.random() < 0.8 else random.randrange(100)
        op = "write %d 7" if random.random() < 0.3 else "read %d"
        f.write(op % (p * PAGE + random.randrange(PAGE)) + "\n")
