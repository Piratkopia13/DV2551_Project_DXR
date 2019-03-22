'''
THIS CODE WAS WRITTEN SPECIFICALLY FOR OUR D3D12 DXR PROJECT, IT MIGHT NOT WORK AS INTENDED IN OTHER CASES.

How to run this file:
Run the file with the input folder as argument, this folder should contain three tsv files.
Each of these files MUST have the same amount of rows and columns, along with two headers. The first header
some information about the system and the second should contain the number of objects tested in each column.

An output file will be generated in the input folder called "averages.tsv" and will contain the averages of the 
three tsv files.
'''


import csv, sys
from os import listdir
from os.path import isfile, join
import os

input_map = sys.argv[1]
averages = [0] * 3
file_header = ''

output_file = input_map + "/averages.tsv"

# Remove the output file if it previously exists (to simplify code)
exists = os.path.isfile(output_file)
if exists:
    os.remove(output_file)

files = [f for f in listdir(input_map) if isfile(join(input_map, f))]
curr_file_index = 0

for input_file in files:
    # Concatinate averages for TLAS, BLAS and VRAM
    with open(input_map + "\\" + input_file, newline='') as inf:
        # Initialize the reader
        reader = csv.reader(inf, delimiter='\t')

        # Get the file header (probably same for all files)
        file_header = next(reader)
        file_header = file_header[0] + " " + file_header[1]
        first_row = next(reader)
        print(f'Number of columns: {len(first_row) - 1}')

        numRows = 0
        averages[curr_file_index] = [0] * (len(first_row) - 3)
        # Iterate through all rows
        for row in reader:
            # Iterate through each value of the row
            for i in range(1, len(row) - 2):
                averages[curr_file_index][i - 1] += float(row[i])
            numRows += 1


        for i in range(0, len(averages[curr_file_index])):
            averages[curr_file_index][i] = averages[curr_file_index][i] / numRows
            print(f'Num objects: {first_row[i + 1]} Average: {averages[curr_file_index][i]}')

    curr_file_index += 1


print(averages)

with open(output_file, "w+", newline='') as of:
    writer = csv.writer(of, delimiter='\t')
    writer.writerow([file_header])
    writer.writerow(["#", "TLAS", "BLAS", "VRAM"])
    
    for i in range(0, len(averages[0])):
        row = []
        row.append(first_row[i + 1])
        row.append(averages[0][i])
        row.append(averages[1][i])
        row.append(averages[2][i])
        writer.writerow(row)