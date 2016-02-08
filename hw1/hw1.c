#include "hw1.h"

int main(int argc, char*argv[])
{
	int opt, iflag = 0, rflag = 0, oflag = 0, uflag = 0;

	/* Parse options */
	while((opt = getopt(argc, argv, "irou")) != -1){
		switch(opt) {
			case 'h':
				USAGE(argv[0]);
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
				USAGE(argv[0]);
				exit(EXIT_FAILURE);
				break;
		}
	}

	if((iflag + oflag + rflag) != 1){
		fprintf(stderr, "Invalid combination of options were provided.\n");
		USAGE(argv[0]);
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

void rflagHandler(int uflag){
	char buf[1024];
	int rtype[32] = {0}, itype[32] = {0}, jtype[32] = {0};
	int opcode;
	int count = 0;
	char *endptr;

	while(fgets(buf, sizeof(buf), stdin) != NULL){
		int rs, rt, rd;
		unsigned long long i = strtoull(buf, &endptr, 16);
		//printf("strtoull(%s) = %llu\n", &buf[0], i);
		//printf("end = %d, %d\n", *endptr, *(endptr + 1));
		if(*(endptr + 1) != '\0' || i > 0xFFFFFFFF){
			fprintf(stderr, "Invalid hex instruction: %llu\n", i);
			exit(EXIT_FAILURE);
		}
		opcode = i >> 26;

		if(opcode == 0){
			//handle r type instructions
			rs = (i >> 21) & 0x1F;
			rt = (i >> 16) & 0x1F;
			rd = (i >> 11) & 0x1F;
			count++; 

			rtype[rs]++;
			rtype[rt]++;
			rtype[rd]++;
		}
		else if(opcode == 2 || opcode == 3){
			//handle i type instructions
			rs = (i >> 21) & 0x1F;
			rt = (i >> 16) & 0x1F;
			count++;

			itype[rs]++;
			itype[rt]++; 
		}
	}


	if(uflag){
		fprintf(stdout,"REG\tUSE\tR-TYPE\tI-Type\tJ-TYPE\tPERCENT\n");
	}

	for(int i = 0; i < 32; i++){
		int regUse = rtype[i] + itype[i] + jtype[i];
		double regPercentage = ((double)regUse / count) * 100;
		//printf("Reg %d: Percentage : %d/%d = %.1f%%\n", i, regUse, regTotal, regPercentage);
		if(uflag){
			// print human readable registers
			const char* regName[32] = {"zero","at", "v0", "v1",
										"a0", "a1", "a2", "a3",
										"t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
										"s0", "s1", "s3", "s3", "s4", "s5", "s6", "s7",
										"t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra"};

			fprintf(stdout, "$%s\t%d\t%d\t%d\t%d\t%.1f%%\n", regName[i], regUse, rtype[i], itype[i], jtype[i], regPercentage);
	}
		else
			fprintf(stdout, "$%d\t%d\t%d\t%d\t%d\t%.1f%%\n", i, regUse, rtype[i], itype[i], jtype[i], regPercentage);
	}


	exit(EXIT_SUCCESS);
}

void oflagHandler(int uflag){
	char buf[1024];
	int opcodeCount[64] = {0};
	int funcCount[64] = {0};
	int opcode, numIns = 0, numFunc = 0;
	char *endptr;


	while(fgets(buf, sizeof(buf), stdin) != NULL){
		unsigned long long i = strtoull(buf, &endptr, 16);
		if(*(endptr + 1) != '\0' || i > 0xFFFFFFFF){
			fprintf(stderr, "Invalid hex instruction: %llu\n", i);
			exit(EXIT_FAILURE);
		}

		numIns++;
		opcode = i >> 26;

		if(opcode == 0){
			opcodeCount[0]++;
			funcCount[(i & 0x3F)]++;
			numFunc++;
		}else{
			opcodeCount[opcode]++;
		} 
			
	}

	if(uflag){
		fprintf(stdout, "OPCODE\tCOUNT\tPERCENTAGE\n");
	}

	for(int i = 0; i < 64; i++){
		fprintf(stdout, "0x%x\t%d\t%.1f%%\n", i, opcodeCount[i], ((double)opcodeCount[i] / numIns) * 100);
	}

	fprintf(stdout, "\n");

	if(uflag)
		fprintf(stdout, "FUNC\tCOUNT\tPERCENTAGE\n");

	for(int i = 0; i < 64; i++){
		double funcPercentage = numFunc == 0 ? 0 : (((double)funcCount[i] / numFunc) * 100);
		fprintf(stdout, "0x%x\t%d\t%.1f%%\n", i, funcCount[i], funcPercentage);
	}

	exit(EXIT_SUCCESS);
}

void iflagHandler(int uflag){
	char buf[1024];
	int itype = 0, rtype = 0, jtype = 0, opcode;
	char *endptr;


	while(fgets(buf, sizeof(buf), stdin) != NULL){
		unsigned long long i = strtoull(buf, &endptr, 16);
		//printf("strtoull(%s) = %llu\n", &buf[0], i);
		//printf("end = %d, %d\n", *endptr, *(endptr + 1));
		if(*(endptr + 1) != '\0' || i > 0xFFFFFFFF){
			fprintf(stderr, "Invalid hex instruction: %llu\n", i);
			exit(EXIT_FAILURE);
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

	if(uflag){
		fprintf(stdout, "TYPE\tCOUNT\tPERCENT\n");
	}

	fprintf(stdout, "I-type\t%d\t%.2f%%\n", itype, ((double)itype / (itype + rtype + jtype) * 100));
	fprintf(stdout, "J-type\t%d\t%.2f%%\n", jtype, ((double)jtype / (itype + rtype + jtype) * 100));
	fprintf(stdout, "R-type\t%d\t%.2f%%\n", rtype, ((double)rtype / (itype + rtype + jtype) * 100));
	
	exit(EXIT_SUCCESS);
}
