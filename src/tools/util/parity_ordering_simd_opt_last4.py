from itertools import permutations

places_lst = list(permutations([0, 0, 0, 0, 4, 4, 4, 4, 32, 32, 32, 32, 36, 36, 36, 36], 4))

parities_lst = [[] for _ in range(64)]

parity_ordering_shuffle_mask = [-1 for _ in range(64)]

for places in places_lst:
    p0, p1, p2, p3 = places
    parities = (p0 ^ p1) | ((p1 ^ p2) >> 1) | ((p2 ^ p3) >> 2)
    parities_lst[parities].append([p0, p1, p2, p3])
    place_count = []
    for elem in [0, 4, 32, 36]:
        place_count.append(places.count(elem))
    sorted_place_count = sorted(place_count)
    need_to_sort = (sorted_place_count == [0, 1, 1, 2]) and (place_count != sorted_place_count)
    sort_lst = [3, 2, 1, 0]
    if need_to_sort: # 2-1-1-0
        sort_lst = []
        no_priority = []
        for i in range(4):
            for j in range(4):
                if i == j:
                    continue
                if places[i] == places[j]:
                    no_priority.append(3 - i)
                    break
            else:
                sort_lst.append(3 - i)
        sort_lst.extend(no_priority)
    #for i in range(4):
    #    sort_lst[i] += 12
    sort_32bit = 0
    for i in range(4):
        sort_32bit <<= 8
        sort_32bit |= sort_lst[i]
    parity_ordering_shuffle_mask[parities] = sort_32bit

for i in range(64):
    print('0x' + hex(parity_ordering_shuffle_mask[i]).upper()[2:], end='')
    print('U, ', end='')
    if i % 8 == 7:
        print('')