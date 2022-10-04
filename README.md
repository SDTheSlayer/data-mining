### Final Report ###
https://github.com/annanyapr/data-mining/blob/main/Data_Mining_Final_report.pdf

### In all the commands replace: ###

1) TYPE with either GML for .gml file, MATRIX for .txt file having graph as adjacency matrix and LINK for .txt file having graph as undirected edges.
2) filePath with the location of graph ; .gml file or .txt file
3) epsilon_value with a float between 0(inclusive) and 1
4) mu_value with an integer greater than 0
#

### Steps for running all evaluations and features: ###

* To compare Running time for adding all edge one by one:
    1) `$ make`
    2) `$ ./comp --TYPE filePath epsilon_value mu_value`

* To compare Running time for deleting all edges one by one:
    1) `$ makeDelete`
    2) `$ ./compDeletion --TYPE filePath epsilon_value mu_value`

* To compare running time for adding random edges:
    1) `$ makecompadd`
    2) `$ ./compadd --TYPE filePath epsilon_value mu_value`

    Use TYPE as LINK as this comparison is done only for large datasets.

* To compare running time for deleting random edges:
    1) `$ makecompdel`
    2) `$ ./compdel --TYPE filePath epsilon_value mu_value`

    Use TYPE as LINK as this comparison is done only for large datasets.

* Comparing Running time of SCAN for multithreading:
    1) `$ makecompscan`
    2) `$ ./compscan --TYPE filePath epsilon_value mu_value`

* To try adding/removing edges/vertices incrementaly to graph:
    1) `$ cd Iscan`
    2) `$ make`
    3) `$ ./main --TYPE filePath epsilon_value mu_value`
    4) Follow further instructions from std out.
#
A total of 7 datasets are used:
1) Example dataset is present in /example/example.gml file
2) Customer dataset is present in /customer/CG1.gml file
3) Polbooks dataset is present in /polbooks/polbooks.gml file
4) Football dataset is present in /football_processed/football.txt file

Datasets GR-QC, COND-MAT and ASTRO-PH can be downloaded as .txt files from links provided in report.
#
Examples to run various executables:
* To give GML file as input:

   `$ ./executable --GML polbooks/polbooks.gml 0.41 2`

* To give adjacency matrix as input from txt file:

    `$ ./executable --MATRIX football/football.txt 0.7 2`
