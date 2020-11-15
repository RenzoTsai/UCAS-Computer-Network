import matplotlib.pyplot as plt
import argparse

def init_parser():
    parser = argparse.ArgumentParser()
    parser.add_argument("--path")
    return parser

def handle_cwnd(f):
    x = []
    y = []
    for line in f:
        line = line.strip('\n')
        if(line.find('cwnd')):
            line = line.split(',',1)
            time = line[0]
            line = line[1].split('cwnd:',1)
            if(len(line) > 1):
                line = line[1].split(' ')
                cwnd = line[0]
                x.append(float(time))
                y.append(int(cwnd))
    return x,y

def handle_rtt(f):
    x = []
    y = []
    for line in f:
        line = line.strip('\n')
        if(line.find('time:')):
            line = line.split(',',1)
            time = line[0]
            line = line[1].split('time=',1)
            if(len(line) > 1):
                line = line[1].split(' ')
                rtt = line[0]
                x.append(float(time))
                y.append(float(rtt))
    return x,y


def handle_qlen(f):
    x = []
    y = []
    for line in f:
        line = line.strip('\n')
        line = line.split(', ',1)
        if(len(line) > 1):
            time = line[0]
            qlen = line[1]
            x.append(float(time))
            y.append(int(qlen))
    return x,y

def print_plt(x, y, type):
    plt.plot(x, y, marker='o', label='lost plot')
    # plt.xticks(x[0:len(x):2], x[0:len(x):2], rotation=45)
    plt.margins(0)
    plt.xlabel("time")
    plt.ylabel(type)
    plt.title(type)
    plt.tick_params(axis="both")
    plt.show()


if __name__ == "__main__":
    parser = init_parser()
    args = parser.parse_args()
    input_txt = args.path
    f = open(input_txt)
    words = input_txt.split("/")
    type = words[1]

    if(type == "qlen.txt"):
        x,y = handle_qlen(f)
    elif(type == "cwnd.txt"):
        x,y = handle_cwnd(f)
    elif(type == "rtt.txt"):
        x,y = handle_rtt(f)
    else:
        x,y = handle_cwnd(f)
    words = type.split(".txt")
    type = words[0]
    print_plt(x,y, type.upper())
    f.close