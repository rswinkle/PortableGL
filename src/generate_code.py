import sys, os, glob

if len(sys.argv)<2:
	print("usage: python3", sys.argv[0], "typename");
	sys.exit();

gen_type = sys.argv[1];

files = glob.glob("cvector_template*.h");
for f in files:
	file_string = open(f).read();
	out_string = file_string.replace("TYPE", gen_type);
	print("Generating cvector_" + gen_type + f[16:]);
	open("cvector_" + gen_type + f[16:], "wt").write(out_string);

