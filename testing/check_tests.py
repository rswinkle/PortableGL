

import sys, os, glob, argparse







if __name__ == "__main__":
    for image in glob.glob('test_output/*.png'):
        name = image.split('/')[1]
        output = open(image, "rb").read()
        expected = open('expected_output/'+name, "rb").read()

        if output != expected:
            print(name + " does not match expected output!")
        else:
            print(name + " passes")

