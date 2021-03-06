# Yet Another Chimeric Read Detector for long reads

![yacrd pipeline presentation](image/pipeline.svg)

Using all-against-all read mapping, yacrd performs:

1. computation of pile-up coverage for each read
2. detection of chimeras

Chimera detection is done as follows:

1. for each region where coverage is smaller or equal than `min_coverage` (default 0), yacrd creates a _gap_.
2. if there is a gap that starts at a position strictly after the beginning of the read and ends strictly before the end of the read, the read is marked as `Chimeric`
3. if gaps length of extremity > 0.8 * read length, the read is marked as `Not_covered`

## Rationale

Long read error-correction tools usually detect and also remove chimeras. But it is difficult to isolate or retrieve information from just this step.

DAStrim (from the [DASCRUBBER suite](https://github.com/thegenemyers/DASCRUBBER) does a similar job to yacrd but relies on a different mapping step, and uses different (likely more advanced) heuristics. Yacrd is simpler and easier to use.

## Input

Any set of long reads (PacBio, Nanopore, anything that can be given to [https://github.com/lh3/minimap2](minimap2)).
yacrd takes the resulting PAF (Pairwise Alignement Format) from minimap2 or MHAP file from some other long reads overlapper as input.

## Requirements

- CMake >= 2.8
- clang >= 3.5 or gcc >= 5.2 (tested with [cmake-cpp-docker](https://github.com/Nercury/cmake-cpp-docker))

## Building

```
git clone https://github.com/natir/yacrd.git
cd yacrd
git checkout v0.02
mkdir build
cd build
cmake ..
make
```

After building, you can move/copy/add yacrd exectuable binary in your PATH

## Usage

1) Run Minimap2: `minimap2 reads.fq reads.fq > mapping.paf` or any other long reads overlapper.
2)

```
usage: yacrd [-h] [-c coverage_min] [-f file_to_filter.(fasta|fastq|mhap|paf) -o output.(fasta|fastq|mhap|paf)] -i mapping.paf

options:
	-h                   Print help message
	-c,--min_coverage    If coverage are minus or equal to this create a gap [0]
	-i,--in              Maping input file in PAF or MHAP format (with .paf or .mhap extension)
    	-f,--filter          File contain data need to be filter (fasta|fastq|paf) output option need to be set
	-o,--output          File where filtered data are write (fasta|fastq|paf) filter option need to be set
```

yacrd writes to standard output (stdout) the id of chimeric or not sufficiently covered reads.

## Output

```
type_of_read	id_in_mapping_file  length_of_read  length_of_gap,begin_pos_of_gap,end_pos_of_gap;length_of_gap,be…
```

### Example

```
Not_covered readA 4599	3782,0,3782
```

Here, readA doesn't have sufficient coverage, there is a zero-coverage region of length 3782bp between positions 0 and 3782.

```
Chimeric    readB   10452   862,1260,2122;3209,4319,7528
```

Here, readB is chimeric with 2 zero-coverage regions: one between bases 1260 and 2122, another between 3209 and 7528.
