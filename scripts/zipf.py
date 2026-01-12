import math
import matplotlib.pyplot as plt

frequencies = []
with open("../data/zipf_statistics.txt", "r", encoding="utf-8") as f:
    lines = f.readlines()
    for i, line in enumerate(lines):
        if i == 0:
            continue
        parts = line.strip().split('\t')
        if len(parts) >= 3:
            try:
                freq = int(parts[2])
                frequencies.append(freq)
            except ValueError:
                continue

frequencies.sort(reverse=True)

ranks = range(1, len(frequencies) + 1)

if frequencies:
    C = frequencies[0]
    zipf = [C / r for r in ranks]

    plt.figure(figsize=(10, 6))
    plt.loglog(ranks, frequencies, 'o-', markersize=3, label="Эмпирическое распределение")
    plt.loglog(ranks, zipf, 'r--', linewidth=2, label="Теоретический закон Ципфа (C/r)")
    
    plt.xlabel("Ранг (log scale)")
    plt.ylabel("Частота (log scale)")
    plt.title("Распределение Ципфа для терминов корпуса")
    plt.legend()
    plt.grid(True, which="both", ls="-", alpha=0.2)
    
    plt.savefig("zipf_distribution.png", dpi=300, bbox_inches='tight')
    plt.show()
    
    print(f"Всего уникальных терминов: {len(frequencies)}")
    print(f"Самый частый термин встречается {frequencies[0]} раз")
    
    print("\nПервые 10 значений:")
    print("Ранг\tЭмпирич.\tТеоретич.\tОтношение")
    for i in range(min(10, len(frequencies))):
        theoretical = C / (i + 1)
        ratio = frequencies[i] / theoretical
        print(f"{i+1}\t{frequencies[i]}\t\t{theoretical:.1f}\t\t{ratio:.2f}")
else:
    print("Нет данных для построения графика")