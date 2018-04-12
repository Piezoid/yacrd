#!/bin/bash

nb_chimeric=$1
min_coverage=${2:-0}

function run_validation {
    mapper=$1
    paf_file=$2

    tmpfile=$(mktemp)
    ../build/yacrd -i $paf_file -c $min_coverage > $tmpfile

    NC=$(grep -c "Not_covered" $tmpfile)
    TP=$(grep "Chimeric" $tmpfile | grep -c "_")
    FP=$(grep "Chimeric" $tmpfile | grep -cv "_")
    FN=$(($nb_chimeric - $TP - $NC))

    rm $tmpfile

    echo "${mapper} :"
    echo -e "\tprecision" $(echo "scale=3; ${TP}/(${TP}+${FP})" | bc)
    echo -e "\trecall:" $(echo "scale=3; ${TP}/(${nb_chimeric})" | bc)
    echo -e "\tF1-score:" $(echo "scale=3; 2*${TP}/(2*${TP}+${FP}+${FN})" | bc)
    echo -e "\tnot covered:" $(echo "scale=3; ${NC}/${nb_chimeric}" | bc)
}

# Make chimeric reads (cached)
chimeric_reads="longislnd_t_roseus.chimeric_${nb_chimeric}.fastq.gz"
[ -f $chimeric_reads ] || ./generation_merge.py longislnd_t_roseus.fastq.gz $chimeric_reads $1

minimap -x ava10k  $chimeric_reads $chimeric_reads > longislnd_t_roseus.minimap1.paf 2> /dev/null
run_validation "minimap" longislnd_t_roseus.minimap1.paf

minimap2 -x ava-pb $chimeric_reads $chimeric_reads > longislnd_t_roseus.minimap2.paf 2> /dev/null
run_validation "minimap2" longislnd_t_roseus.minimap2.paf
