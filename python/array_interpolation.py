# Pre Interp
# [d d d d d]
# [d d d d d]
# [d d d d d]

# Post Interp
# [d i d i d i d i d]
# [i i i i i i i i i]
# [d i d i d i d i d]
# [i i i i i i i i i]
# [d i d i d i d i d]

# interp(initial,final) -> float
# interp((i,j),(i,j+1))
# interp((i,j),(i,j+1))
# interp((i,j),(i+1,j))
# interp((i,j),(i+1,j+1))
# Increment by 1 and repeat
# if i >= num_columns-1:
#     i = 0

def interp(a,b):
    return (a+b)/2

def interp_array(array):
    tmp = []
    post_interp = []

    i = 0
    j = 0
    while True:
        if i >= len(array[0])-1:
            tmp.append(array[j][i])
            post_interp.append(tmp)
            tmp = []

            i = 0
            j += 1
            if j >= len(array):
                break
            
        tmp.append(array[j][i])
        tmp.append(interp(array[j][i], array[j][i+1]))
        i += 1
    return post_interp


def interp_array2d(array):

    array1d = interp_array(array)

    tmp = []
    post_interp = []

    i = 0
    j = 0
    while True:
        if i >= len(array1d)-1:
            tmp.append(array1d[i][j])
            post_interp.append(tmp)
            tmp = []
            i = 0
            j += 1
            if j >= len(array1d[0]):
                break
        tmp.append(array1d[i][j])
        tmp.append(interp(array1d[i][j], array1d[i+1][j]))
        i += 1
    return [list(i) for i in zip(*post_interp)]


def main():
    pre_interp = \
    [
        [ 1 , 0 , 1 ],
        [ 0 , 2 , 1 ],
        [ 5 , 3 , 1 ]
    ]
    post_interp = interp_array2d(pre_interp)
    print(post_interp)

if __name__ == "__main__":
    main()