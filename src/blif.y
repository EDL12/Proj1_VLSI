%{
/******************************************************************\ 
 *
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 *
 * This code is provided as is, with no warranty implied or expressed.
 * Use at your own risk.  This software may contain bugs which could
 * cause it to malfunction in possibly dangerous ways.
 *  
 * Neither the author, nor any affiliates of the author are responsible for 
 * any damages caused by use of this software.
 *
 * Copyright:  Narayanan Krishnamurthy, Heling Yi, 1997
 *
\******************************************************************/


/******************************************************************\ 
 *
 * This code was developed as part of a project in the digital system 
 * simulation class (Spring 1997 ) taught by 
 * Dr. Nur Touba at The University of Texas at Austin.
 * The project is Path Delay Fault simulation. This file is a "yacc" 
 * file that describes the grammar of a BLIF file that has 
 * been mapped to a technology library containing only 2-input or less
 * combinational gates. The parser parses the BLIF files and calls
 * the Add_Gate() and Build_Ckt() routines in "parse_ckt.c"
\******************************************************************/

#include <stdio.h>
#include <string.h> 
#include "read_ckt.h"

/******************************************************************\
 * STAND_ALONE has to be defined if the parser has to be
 * run in a stand_alone mode with its own "main()" function
 * Otherwise, the function ParseAndBuildCkt() function
 * is called from the read_circuit() function in read_circuit.c
\******************************************************************/

#ifndef STAND_ALONE
int ParseAndBuildCkt(FILE *);
#endif

/******************************************************************\
 * Add_Gate() and Build_Ckt() are functions being called from
 * this yacc file. The functions are defined in "read_ckt.cc"
\******************************************************************/
/*extern  void Add_Gate(Gate_Info_t *);*/
/*extern void Build_Ckt(void);*/

/******************************************************************\
 * DEBUG_MESSAGES  has to be defined to generate debug messages
 * as the BLIF file is being parsed
\******************************************************************/
#ifdef DEBUG_MESSAGES
void PrintGateInfo(void);
#endif


extern char *nameBuf;
extern char *funcCover;

/******************************************************************\
 * NETLIST_DUMP has to be defined to generate the "Output_Netlist"
 * which is a parsed output of the input BLIF file
\******************************************************************/
#ifdef NETLIST_DUMP
FILE *GateFile;
void DumpGateToFile(FILE *, Gate_Info_t *);
#endif

Gate_Info_t		gateInfo; 

/******************************************************************\
 * The following variables and data_structures are defined for 
 * local use within the yacc file. Added an additional UNKNOWN
 * gate because while parsing the signal list of the gate the
 * the gate type is unknown until the cover of the function
 * is read.

 * Currently handles 2-ip AND, OR, NAND, NOR and 1-i/p INV,
 * 1-i/p BUF, constant 0 (GND) and constant 1 (VCC) nodes.
 * PI's and PO's are also handled as separate nodes.
\******************************************************************/
#define MAX_GATE_TYPES 11 

/******************************************************************\
 * Commented this out because Dr. Touba said that it was not necesary
 * consider a generic CONSTANT node but just a PO constant node.
struct Gate_Type_t {
	char *name;
	char *cover;
	int  total;
} gate_type[MAX_GATE_TYPES] = {
                        "AND",  "11 1", 0,
		      			"OR",   "1- 1", 0,
				       	"NAND", "0- 1", 0,
				        "NOR", 	"00 1", 0,
				      	"INV",  "0 1", 0,
				       	"BUF",  "1 1", 0,
				      	"CONSTANT_1", "1", 0,
				      	"CONSTANT_0", "", 0,
				      	"PI", 	"", 0,
				       	"PO", 	"", 0,
			                "UNKNOWN", "", 0
			    };
\******************************************************************/

struct Gate_Type_t {
	char *name;
	char *cover;
	int  total;
} gate_type[MAX_GATE_TYPES] = {
                        "AND",  "11 1", 0,
		      			"OR",   "1- 1", 0,
				       	"NAND", "0- 1", 0,
				        "NOR", 	"00 1", 0,
				      	"INV",  "0 1", 0,
				       	"BUF",  "1 1", 0,
				      	"PI", 	"", 0,
				       	"PO", 	"", 0,
				      	"PO_GND", "", 0,
				      	"PO_VCC", "1", 0,
			                "UNKNOWN", "", 0
			    };
Gate_Type_Enum		nodeKind = UNKNOWN;   
int			signalNum; 
char                    **signalArray;

%}

%union {
  int ival;
  char* sval;
}

%start circuit_module

%token		    BLIF_MODEL BLIF_INPUTS BLIF_OUTPUTS 
%token		    BLIF_NAMES BLIF_END
%token	<sval>	    NAME  GATE_COVER
%token              LINE_CONT BLIF_SEQ BLIF_FSM

%type   <sval>      signal_list
%type   <sval>      cover_list

%%

circuit_module:  decl_model decl_ip_list decl_op_list commandlist BLIF_END

/******************************************************************\
 * DEBUG_MESSAGES  has to be defined to generate debug messages
 * as the BLIF file is being parsed
\******************************************************************/
                {
#ifdef DEBUG_MESSAGES
		  printf("Circuit Model successfully parsed\n");
#endif 
		}
                | decl_model decl_op_list decl_ip_list commandlist BLIF_END
                {
#ifdef DEBUG_MESSAGES
		  printf("Circuit Model successfully parsed\n");
#endif 
		}
		| invalid_ckt
		{
#ifdef DEBUG_MESSAGES
		  printf("Not a Combinational Circuit\n");
#endif 
		}
                ;

decl_model:     BLIF_MODEL NAME 
                { 
#ifdef DEBUG_MESSAGES
		  printf(".model %s parsed\n", $2); 
#endif
		}
                ;

decl_ip_list:   decl_ip_list  BLIF_INPUTS signal_list
                {
#ifdef DEBUG_MESSAGES
                 printf(".input parsed - 4\n");
#endif
		 gate_type[PI].total++;
	        }
                | decl_ip_list BLIF_INPUTS
		{
#ifdef DEBUG_MESSAGES
                 printf(".input parsed - 3\n");
#endif 
		 gate_type[PI].total++;
		}
		| decl_ip_list signal_list
 		{
#ifdef DEBUG_MESSAGES
		 printf(".input + signal_list parsed\n");
		 printf("Total PI's %d\n", gate_type[PI].total);
#endif 
		 signalNum =0;
		 gateInfo.GateType = nodeKind = UNKNOWN;		}
		| BLIF_INPUTS 
                { 
#ifdef DEBUG_MESSAGES
		  printf(".input parsed\n"); 
#endif 
		 gateInfo.GateType = nodeKind = PI;
		 gateInfo.NumOfNode = 1;
		 gate_type[PI].total = 0;
		}
		;		


decl_op_list:   decl_op_list BLIF_OUTPUTS signal_list
                {
#ifdef DEBUG_MESSAGES
                 printf(".output parsed - 1\n");
#endif 
		}
                | decl_op_list BLIF_OUTPUTS
		{
#ifdef DEBUG_MESSAGES
                 printf(".output parsed - 2\n");
#endif 
		}
		| decl_op_list signal_list
		{
#ifdef DEBUG_MESSAGES
		 printf(".output + signal_list parsed\n");
		 printf("Total PO's %d\n", gate_type[PO].total);
#endif 
		 signalNum =0;
		 gateInfo.GateType = nodeKind = UNKNOWN;		}
		| BLIF_OUTPUTS 
                { 
#ifdef DEBUG_MESSAGES
		 printf(".output parsed\n"); 
#endif 
		 gateInfo.GateType = nodeKind = PO;
		 gateInfo.NumOfNode = 1;
		 gate_type[PO].total = 0;
		}
		;


signal_list:    signal_list NAME LINE_CONT
                {
#ifdef DEBUG_MESSAGES
		  printf("2 or more signals with line_cont parsed\n %dnd signal%s\n", signalNum + 1, $2);

#endif 
		  $$ = $2;
		if((gateInfo.GateType == PI) || (gateInfo.GateType == PO)){ 
		  gateInfo.NameOfNode = (char **) malloc(
				    gateInfo.NumOfNode*sizeof(char *));
		  gateInfo.NameOfNode[0] = $2;
		  gate_type[gateInfo.GateType].total++;
#ifdef NETLIST_DUMP
		  DumpGateToFile(GateFile, &gateInfo);
#endif
                       Add_Gate(&gateInfo); 
		        free(gateInfo.NameOfNode);
		}
		 else {
		   if((signalArray = (char **) realloc(signalArray, 
					   (signalNum+1)*sizeof(char *)))
		                    == NULL)
		      perror("Realloc failed on line_cont signalArray\n");
		   signalArray[signalNum] = $2;
		   signalNum++;
		 }
		}
                | signal_list NAME
                {
#ifdef DEBUG_MESSAGES
		  printf("2 or more signal names parsed\n %dnd signal %s\n", 
			 signalNum+1, $2);
#endif 
		  $$ = $2;
		if((gateInfo.GateType == PI) || (gateInfo.GateType == PO)){ 
		  gateInfo.NameOfNode = (char **) malloc(
			   gateInfo.NumOfNode*sizeof(char *));
		  gateInfo.NameOfNode[0] = $2;
		  gate_type[gateInfo.GateType].total++;
#ifdef DEBUG_MESSAGES
		  PrintGateInfo();
#endif 
#ifdef NETLIST_DUMP
		  DumpGateToFile(GateFile, &gateInfo);		  
#endif
            Add_Gate(&gateInfo);
		   free(gateInfo.NameOfNode[0]);
		   free(gateInfo.NameOfNode);
		}
		 else {
		   int i;
#ifdef DEBUG_MESSAGES
		   printf("Before realloc\n");
		   for(i=0; i<signalNum; i++){
		     printf("%d signal: %s\n", i+1, 
			    signalArray[i]);
		   }
#endif 

		   if((signalArray = (char **) realloc(signalArray, 
					   (signalNum+1)*sizeof(char *))) 
		                                 == NULL)
		      perror("Realloc failed on signalArray\n");
		   signalArray[signalNum] = $2;
#ifdef DEBUG_MESSAGES
		   printf("After realloc\n");
		   for(i=0; i<signalNum+1; i++){
		     printf("%d signal: %s\n", i+1, 
			    signalArray[i]);
		   }
#endif 
		   signalNum++;
		 }
		}
                | NAME
                {
#ifdef DEBUG_MESSAGES
		 printf("signal_name parsed %s\n", $1);
#endif 
		 $$ = $1;
		 if((gateInfo.GateType == PI) || (gateInfo.GateType == PO)){
		   gateInfo.NameOfNode = (char **) malloc(
				gateInfo.NumOfNode*sizeof(char *));
		   gateInfo.NameOfNode[0] = $1;
		   gate_type[gateInfo.GateType].total++;
#ifdef DEBUG_MESSAGES
		   PrintGateInfo();
#endif 
#ifdef NETLIST_DUMP
		   DumpGateToFile(GateFile, &gateInfo);		    
#endif
		     Add_Gate(&gateInfo); 
			 free(gateInfo.NameOfNode[0]);
		    free(gateInfo.NameOfNode);
		 }
		 else {  
#ifdef DEBUG_MESSAGES
		        printf("Gate Type %s\n", 
			       gate_type[gateInfo.GateType].name);
#endif 
		   if((signalArray = (char **) malloc((signalNum+1)*
				    sizeof(char *))) == NULL)
	                perror("malloc fail on signalArray\n");
		      else{
		 	signalArray[signalNum] = $1;
#ifdef DEBUG_MESSAGES
			printf("Signal %s\n", signalArray[signalNum]);
#endif 
		      }

		 }
		 signalNum = 1;
                }
                ;

commandlist:	commandlist command
				| command
				;


command:		logic_gate
				;		

logic_gate:    BLIF_NAMES  signal_list cover_list
                {
                if(signalNum == 1){
                   if( $3 == (char *)0){
#ifdef DEBUG_MESSAGES
/*		       printf("CONSTANT 0 node %s\n", $2); */
		       printf("PO_GND node %s\n", $2);
#endif 
                		/* constant node 0 */
/*		         gateInfo.GateType = nodeKind = CONSTANT_0; */
		         gateInfo.GateType = nodeKind = PO_GND;
                   } 
                   else{
                    free($3);  /* freeing the cover storage for "1" */
#ifdef DEBUG_MESSAGES
/*			printf("CONSTANT 1 node %s\n", $2); */
			printf("PO_VCC node %s\n", $2);
#endif 
                		/* constant node 1 */
/*		 	gateInfo.GateType = nodeKind = CONSTANT_1; */
		 	gateInfo.GateType = nodeKind = PO_VCC;
                   }
		 	 gateInfo.NumOfNode = 1;		 
		         gateInfo.NameOfNode = signalArray;
		   	 gate_type[gateInfo.GateType].total++;
#ifdef DEBUG_MESSAGES
		         PrintGateInfo();
#endif 
#ifdef NETLIST_DUMP
			 DumpGateToFile(GateFile, &gateInfo);   
#endif
                         Add_Gate(&gateInfo); 
		   	  free(gateInfo.NameOfNode[0]);
		   	  free(gateInfo.NameOfNode);
                }
                else 
                if(signalNum > 1){
                int i;
				if(nodeKind != NAND && nodeKind != OR)
                    free($3);  /* freeing the cover storage */  
		 	gateInfo.NumOfNode = signalNum;		 
		   	gateInfo.NameOfNode = signalArray; 
		 	gateInfo.GateType = nodeKind;
			/* need to check for UNKNOWN gate and if it comes up
			 * may need to exit
			 */
#ifdef DEBUG_MESSAGES
			PrintGateInfo();
#endif 
#ifdef NETLIST_DUMP
			DumpGateToFile(GateFile, &gateInfo);
#endif
			if(signalNum > 3 || nodeKind == UNKNOWN){
				printf("Invalid Gate in Circuit\n");
				exit(-1);
			}
		    	  Add_Gate(&gateInfo); 

			  for(i=0; i<signalNum; i++){
		   		 free(gateInfo.NameOfNode[i]);
			  }
		   	 free(gateInfo.NameOfNode);
                }
#ifdef DEBUG_MESSAGES
		  printf("logic gate parsed\n");
#endif 
		  gateInfo.GateType = nodeKind = UNKNOWN;
		  signalNum = 0;
		  signalArray = NULL;

		}
                ;


cover_list :    cover_list  GATE_COVER
                {
#ifdef DEBUG_MESSAGES
                  switch(nodeKind){
                    case OR:
		       printf("OR gate parsed\n%s\n%s\n", $1, $2);
                    break;
                    case NAND:
		       printf("NAND gate parsed\n%s\n%s\n", $1, $2);
                    break;
                  }
#endif 
                   free($1);
                   free($2);

		}
                | GATE_COVER
                {
		  int i;

		  $$ = $1;

		  /***************************************************\ 
		   * check till only gates that actually have a cover
		   * such as 
		   *			.names [1]  or  .names [2] [4] [5]
 		   *			1 				11 1
		   * These gates are gates 0 (AND) to 5 (BUF) and
		   * node PO_VCC
		   * PO_GND appears as follows 
		   *			.names [1]  
		   *
		   * This is because we decide the logic gate by actually
		   * doing a table lookup (compare the cover with the
		   * one in the table to decide the nodeKind)
		   \***************************************************/
/*		  for(i=0; i<MAX_GATE_TYPES-4; i++){ */
		  for(i=0; i<MAX_GATE_TYPES-5; i++){
		  	if(!strcmp(funcCover, gate_type[i].cover)){
#ifdef DEBUG_MESSAGES
		   	    printf("%s gate %s\n", gate_type[i].name, $1);
#endif 
		   	    nodeKind = i;
		   	    break;
		  	}
		  }
		  if(i == MAX_GATE_TYPES - 5){ /* ie loop exits wo breaking */
		  	if(!strcmp(funcCover, gate_type[PO_VCC].cover)){
#ifdef DEBUG_MESSAGES
		   	    printf("%s gate %s\n", gate_type[i].name, $1);
#endif 
				nodeKind = PO_VCC;
			}
		  }
		}
                | 
                { $$ = (char *) 0; }
                ;

invalid_ckt :	BLIF_SEQ 
				{
					printf("Sequential ckt\n");
				}
				| BLIF_FSM
				{
					printf("FSM ckt\n");
				}
				;

%%
extern FILE *yyin;

#ifdef STAND_ALONE
main(int argc, char **argv)
{
	FILE	*file;
#else
int ParseAndBuildCkt(FILE *file)
{
#endif
#ifdef NETLIST_DUMP
	char    output_file[] = "Output_Netlist";
#endif

#ifdef STAND_ALONE
	if (argc == 2){
		file = fopen(argv[1], "r");
		if (!file) {
			fprintf(stderr, "could not open %s\n", argv[1]);
			exit(1);
		}
	}
	else{
	    printf("Usage::  parse <filename>\n");
	    exit(0);
        }
#endif
		yyin = file;
#ifdef NETLIST_DUMP
		GateFile = fopen(output_file,"wt");
		if (!GateFile) {
			fprintf(stderr, "could not open %s\n", output_file);
			exit(1);
		}
#endif
	while (!feof(yyin)) {
		yyparse();
	}
  	Build_Ckt();
	printf("Circuit Parsed and Built successfully\n");
#ifdef STAND_ALONE
        fclose(file);
#endif
#ifdef NETLIST_DUMP
	fclose(GateFile);
#endif
        return 1;
}

yyerror(char *s)
{
	fprintf(stderr, "%s\n", s);
}

#ifdef DEBUG_MESSAGES 

void PrintGateInfo(void)
{
 int i;

 printf("Gate Type %s\n", gate_type[gateInfo.GateType].name); 
 for (i=0; i<gateInfo.NumOfNode-1; i++)
   printf("i/p name %s\n", gateInfo.NameOfNode[i]);
 printf("o/p name %s\n", gateInfo.NameOfNode[i]);
}

#endif

#ifdef NETLIST_DUMP
void DumpGateToFile(FILE *file, Gate_Info_t *gate)
{
  int i;

  fprintf(file, "%d\n", gate->GateType);
  /*  fprintf(file, "%s\n", gate_type[gate->GateType].name); */
  fprintf(file, "%d\n", gate->NumOfNode);
  for (i=0; i<gate->NumOfNode; i++)
    fprintf(file, "%s\n", gate->NameOfNode[i]);
  
}
#endif

void read_circuit(FILE *ckt_file)
{
  ParseAndBuildCkt(ckt_file);
} 

