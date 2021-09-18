

import sys, os, glob, argparse







if __name__ == "__main__":

    results = []
    for image in glob.glob('test_output/*.png'):
        name = image.split('/')[1]
        output = open(image, "rb").read()
        expected = open('expected_output/'+name, "rb").read()

        results.append((name, output == expected))

        #if output != expected:
        #    print(name + " does not match expected output!")
        #else:
        #    print(name + " passes")

    # somehow use any/all
    had_failure = False
    for test in results:
        if not test[1]:
            print(test[0], "FAILED")
            had_failure = True

    if not had_failure:
        print("All tests passed")

