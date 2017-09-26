import collections


def split_on_chunks(x, chunk_size):
    l = [x[i:i+chunk_size] for i in range(0, len(x), chunk_size)]
    return l


def generate_chunks(deque, s, size):
    result = []
    while True:
        chunks = split_on_chunks(''.join(deque), size)

        if chunks[0].count('*') == 0:
            for s in reversed(chunks):
                if s.count('*') == len(s):
                    chunks.remove(s)
            result.append(chunks)
            break

        for i, s in enumerate(chunks):
            if s.count('*') == len(s):
                chunks.remove(s)
            else:
                chunks[i] += '*' * (size - len(s))

        result.append(chunks)
        deque.append('*')

    return result


def generate_sequences(s, size):
    deque = collections.deque(maxlen=len(s) + size - 1)
    for a in range(size):
        deque.append('*')

    for i in range(0, len(s)):
        deque.append(s[i])

    return generate_chunks(deque, s, size)


def inc_str(s):
    return str(int(s) + 1)


def dec_str(s):
    return str(int(s) - 1)


# corner case for '9'
# *9, 2*        -> 19, 20
# **9', '21*    -> 209, 210
# *99, 2**      -> 199, 200
# *29, 1**      -> 129, 130
# ***9', '222*  -> 2219, 2220
# **9, 17*      -> 169, 170
# 89, 9*        -> 89, 90
# 989, 9**      -> 989, 990
# *9, 78        -> 79, 78
# *9, 80        -> 79, 80
# '*19', '1**'  -> 119, 120
# '*99', '1**'  -> 099, 100
def ninths(first, second):
    stars_in_first = first.find('*') != -1
    stars_in_second = second.find('*') != -1
    num_stars_second = second.count('*')

    if stars_in_first:
        last_num = first[first.rfind('*') + 1:]
        sl = list(second)
        left_part_second = sl[0:-num_stars_second]

        if stars_in_second:
            sl[-len(last_num):] = inc_str(last_num)[-num_stars_second:]
            second = ''.join(sl)

        if len(left_part_second) == 0:
            left_part_second = second[0:first.count('*')]

        fl = list(first)
        if len(left_part_second) != 1:
            fl[0:len(left_part_second)] = list(dec_str(''.join(left_part_second)))
        else:
            fl[0:len(left_part_second)] = list(''.join(left_part_second))

        first = ''.join(fl)
    elif stars_in_second:
        new_last_num = first[-num_stars_second:]
        sl = list(second)
        sl[-len(new_last_num):] = inc_str(new_last_num)[-num_stars_second:]
        second = ''.join(sl)

    return first, second


def replace_stars(first, second):
    n_stars_first = first.count('*')
    n_start_second = second.count('*')

    if second[0] == '0':
        second = second.replace('*' * n_start_second, '0' * n_start_second)
        first = first.replace('*' * n_stars_first, '0')
        return first, second

    if first[-1] == '9':
        return ninths(first, second)

    first_replaced = first.replace('*' * n_stars_first, second[:n_stars_first])
    if n_start_second != 0:
        second_replaced = second.replace('*' * n_start_second, first[len(first) - n_start_second:])
        # '**1', '00*' -> '1', '2' (BAD!), '001', '002' - RIGHT
        if second_replaced[0] != '0':
            second_replaced = inc_str(second_replaced)
        return first_replaced, second_replaced
    else:
        return first_replaced, second


def check_sequence_increasing_by_one(sequence):
    try:
        for f, s in zip(sequence[0::1], sequence[1::1]):
            if int(f) + 1 != int(s):
                return False
        return True
    except:
        return False


def find_beginning_num(s):
    for order in range(1, len(s) + 1):
        seqs = generate_sequences(s, order)
        beginnings = []
        for seq in seqs:
            for i in range(0, len(seq) - 1):
                if len(seq[i + 1]) > 1 and seq[i + 1][0] == '0':
                    continue

                res = replace_stars(seq[i], seq[i + 1])
                seq[i] = res[0]
                seq[i + 1] = res[1]
            if check_sequence_increasing_by_one(seq):
                beginnings.append(seq)

        if len(beginnings) > 0:
            beginnings = sorted(beginnings)
            shift = ''.join(beginnings[0]).find(s)
            return beginnings[0], shift


def get_pos_in_sequence(num):
    remainder = 0
    for i in range(len(num) - 1, 0, -1):
        remainder += ((10 ** (i - 1) * 9) * i)
    return abs(10 ** (len(num) - 1) - int(num)) * len(num) + remainder + 1


def find_fast(s):
    num = find_beginning_num(s)
    return get_pos_in_sequence(num[0][0]) + num[1]


if __name__ == '__main__':
    numbers = []
    number = ''

    while True:
        number = raw_input()
        if number is not '':
            numbers.append(int(number))
        else:
            break

    for number in numbers:
        print find_fast(str(number))
