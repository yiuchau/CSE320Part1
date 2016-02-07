#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>


#define USAGE(name) fprintf(stderr,	"\n%s Usage:	./mstat [OPTION]\n		./mstat -h 			Displays this help menu\n		./mstat -i [-u]		Displays statistics about instruction types.\n		./mstat -r [-u]		Displays information about the registers.\n		./mstat -o [-u] 	Displays number and percentage of opcodes used.\n\nOptional flags:\n			-u 		Displays human readable headers for the different outputs.\n", name);

void rflagHandler(int uflag){

	exit(EXIT_SUCCESS);
}

void oflagHandler(int uflag){

	exit(EXIT_SUCCESS);
}

void iflagHandler(int uflag){
	char buf[1024];
	int itype = 0, rtype = 0, jtype = 0, opcode;
	char *endptr;

	if(uflag){
		fprintf(stdout, "TYPE\tCOUNT\tPERCENT\n");
	}

	while(fgets(buf, sizeof(buf), stdin) != NULL){
		unsigned long long i = strtoull(buf, &endptr, 16);
		//printf("strtoull(%s) = %llu\n", &buf[0], i);
		//printf("end = %d, %d\n", *endptr, *(endptr + 1));
		if(*(endptr + 1) != '\0'){
			fprintf(stderr, "Invalid hex instruction: %llu\n", i);
		}
		opcode = i >> 26;

		if(opcode == 0)
			rtype++;
		else 
			if(opcode == 2 || opcode == 3)
				jtype++;
			else
				itype++;
	}

	fprintf(stdout, "I-type\t%d\t%.2f%%\n", itype, ((double)itype / (itype + rtype + jtype) * 100));
	fprintf(stdout, "J-type\t%d\t%.2f%%\n", jtype, ((double)jtype / (itype + rtype + jtype) * 100));
	fprintf(stdout, "R-type\t%d\t%.2f%%\n", rtype, ((double)rtype / (itype + rtype + jtype) * 100));
	
	exit(EXIT_SUCCESS);
}

int main(int argc, char*argv[])
{
	int opt, iflag = 0, rflag = 0, oflag = 0, uflag = 0;

	/* Parse options */
	while((opt = getopt(argc, argv, "irou")) != -1){
		switch(opt) {
			case 'h':
				//USAGE();
				exit(EXIT_SUCCESS);
				break;
			case 'i':
				iflag++;
				break;
			case 'r':
				rflag++;
				break;
			case 'o':
				oflag++;
				break;
			case 'u':
				uflag++;
				break;
			case '?':
				/* Let this case fall down to default
				 * handled during bad option.
				 */
			default:
				fprintf(stderr,"A bad option was provided.\n");
				//USAGE();
				exit(EXIT_FAILURE);
				break;
		}
	}

	if((iflag + oflag + rflag) != 1){
		fprintf(stderr, "Invalid combination of options were provided.\n");
		//USAGE();
		exit(EXIT_FAILURE);
	}

	if(iflag){
		iflagHandler(uflag);
	}

	if(rflag){
		rflagHandler(uflag);
	}

	if(oflag){
		oflagHandler(uflag);
	}


	exit(EXIT_SUCCESS);

}