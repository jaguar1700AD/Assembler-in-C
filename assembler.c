#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

int decipher_word(char word[], int index, int PC); // Used in 1st pass. Returns the bit count of the word
int decipher_word2(char word[], int index, int PC); // Used in 2nd pass. Returns the bit count of the word
int is_present_in(char str[], char instrs[][5]); // Checks whether str is presnt in instrs array
void output_word_op(int index); // outputs the word with the given index (in instrs array) in the opTable
void label_address(char label[], char addr[]); // Returns the address of the label by checking the symbol table
void print_hex(char word[], int mode); // mode 0 for register, 1 for value, 2 for address
char* hex_digit_to_bin(char c); // returns the 4 digit binary string corresponding to a hex digit

char instrs[16][5];  // Upto 16 instructions are possible
char ops[16][5]; // The max length of each instruction and opcode can be 4
int N = 0; // the no of instruction is N

bool encountered[16];
FILE *input, *sym_out, *op_out, *prog_out1, *out;

void main()
{
	input = fopen("machine_opcode_mapping.txt", "r");
	if (input == NULL)
	{	
		printf("File not opened\n");
		exit(0);
	}

	char word[10]; // The max size of any word(even labels) should thus be 9
	char ch;
	int index = 0; 

	// Reading the machine opcode mappping into arrays ----------------
	while (true)
	{
		index = 0;
		while((ch = fgetc(input)) != ' ')
			word[index++] = ch;
		word[index] = '\0';
		strcpy(instrs[N], word);

		index = 0;
		while((ch = fgetc(input)) != EOF && ch != '\n' && ch != ' ')
			word[index++] = ch;
		word[index] = '\0';
		strcpy(ops[N++], word);

		if ( (ch = fgetc(input)) != EOF)
			fseek(input, -1, SEEK_CUR);
		else
			break;
	}

	fclose(input);
	//-----------------------------------------------------------------
	
	for(int i = 0; i < N; i++) encountered[i] = false;

	input = fopen("input.asm", "r");
	sym_out = fopen("symTable.txt", "w");
	op_out = fopen("opTable.txt", "w");
	prog_out1 = fopen("prog_out1.txt", "w");
	
	bool reading_word = true;
	int PC = 0; // Current address assuming byte addressable memory
	int bit_count = 0; // No of bits of instruction in current line encountered uptil now
	index = 0;
	word[0] = '\0';

	while((ch = fgetc(input)) != EOF)
	{
		if (ch == ' ' || ch == '\n' || ch == ',')
		{
			if (reading_word)
			{
				reading_word = false;
				word[index] = '\0';
				bit_count += decipher_word(word, index - 1, PC);
			}

			if(ch == '\n')
			{
				if (bit_count != 0)
					fprintf(prog_out1, "\n");
				if (bit_count % 8 == 0)
					PC += bit_count / 8;
				else
					PC += bit_count / 8 + 1;
				
				bit_count = 0;
			}
		}
		else
		{
			if(!reading_word)
			{
				reading_word = true;
				index = 0; 
			}
			word[index++] = ch;
		}
	}

	fclose(input);
	fclose(sym_out);
	fclose(op_out);
	fclose(prog_out1);

	// --------------------------------------------------  2nd Pass  ----------------------------------------------------------------------------

	input = fopen("prog_out1.txt", "r");
	sym_out = fopen("symTable.txt", "r");
	out = fopen("output.o", "w");
	
	fprintf(out, "ADDRESS \t INSTRUCTION \n\n");

	reading_word = true;
	index = 0; 
	PC = 0; // Current address assuming byte addressable memory
	bit_count = 0; // No of bits of instruction in current line encountered uptil now

	fprintf(out, "%04X \t ", PC);
	while((ch = fgetc(input)) != EOF)
	{
		if (ch == ' ' || ch == '\n' || ch == ',')
		{
			if (reading_word)
			{
				reading_word = false;
				word[index] = '\0';
				bit_count += decipher_word2(word, index - 1, PC);
			}

			if(ch == '\n')
			{
				if (bit_count % 8 == 0)
					PC += bit_count / 8;
				else
					PC += bit_count / 8 + 1;
				
				bit_count = 0;

				fprintf(out, "\n");

				if ( (ch = fgetc(input)) != EOF)
				{	
					fseek(input, -1, SEEK_CUR);
					fprintf(out, "%04X \t ", PC);
				}
			}
		}
		else
		{
			if(!reading_word)
			{
				reading_word = true;
				index = 0; 
			}
			word[index++] = ch;
		}
	}

	fclose(input);
	fclose(sym_out);

	input = fopen("prog_out1.txt", "w");
	fclose(input);
}

int decipher_word(char word[], int index, int PC) // index is the last index of the word (the one just before null)
{
	int comp_ind = is_present_in(word, instrs);
	
	// Print in optable
	if (strcmp(word, "LOOP") == 0) // LOOP
	{
		output_word_op(is_present_in("SUB", instrs));
		output_word_op(is_present_in("JNZ", instrs));
	}
	else
		output_word_op(comp_ind);


	if (strcmp(word, "MUL") == 0) // MUL 
	{
		fprintf(prog_out1, "%s R1 ", word);
		return 9;
	}
	else if (strcmp(word, "LOOP") == 0) // LOOP
	{
		// Read the label -------------------------------------
		
		char ch; char label[10]; int index = 0;
		while((ch = fgetc(input)) == ' '); // Skip the spaces after LOOP
		do
		{
			label[index++] = ch;   // Read the label

		}while((ch = fgetc(input)) != ' ' && ch != '\n');
		label[index] = '\0';

		//-------------------------------------------------------

		fprintf(prog_out1, "SUB R31 0001H \n");
		fprintf(prog_out1, "JNZ %s \n", label);

		return 56;
	}
	else if (strcmp(word, "JMP") == 0) // JMP 
	{
		fprintf(prog_out1, "%s ", word);
		return 20;
	}
	else if (comp_ind != -1)
	{
		fprintf(prog_out1, "%s ", word);
		return 4;
	}
	else if (word[0] == 'R') // Is a register
	{
		fprintf(prog_out1, "%s ", word);
		return 5;
	}
	else if (word[index] == 'H') // value
	{
		fprintf(prog_out1, "%s ", word);
		return 16;
	}
	else if (word[index] == ':') // label defined
	{
		fprintf(sym_out, "%s %04XH\n", word, PC);	// Symbol table output
		return 0;
	}
	else if (strcmp(word, "START") == 0 || strcmp(word, "END") == 0) // start or end detected; ignore
		return 0;	
	else   // label used
	{
		fprintf(prog_out1, "%s ", word);
		return 0;
	}
}

int decipher_word2(char word[], int index, int PC) // index is the last index of the word (the one just before null)
{
	int comp_ind = is_present_in(word, instrs);

	if (comp_ind != -1)
	{
		fprintf(out, "%s ", ops[comp_ind]);
		return 4;
	}
	else if (word[0] == 'R') // Is a register
	{
		// Take the number part from the register ------------

		char val[3]; int i;
		for(i = 0; word[i+1] != '\0'; i++) val[i] = word[i+1];
		val[i] = '\0';
		
		//----------------------------------------------------

		print_hex(val, 0);

		return 5;
	}
	else if (word[index] == 'H') // value
	{
		word[index] = '\0';
		print_hex(word, 1);
		return 16;
	}
	else    // label used
	{
		char addr[10]; label_address(word, addr);
		if (addr[0] == '\0')
		{
			fprintf(out, "\n\nError in label used\n");
			exit(0);
		}

		print_hex(addr, 2);
		
		return 16;
	}
}

int is_present_in(char str[], char instrs[][5])
{
	for(int i = 0; i < N; i++)
	{
		if (strcmp(str, instrs[i]) == 0)
			return i;
	}
	return -1;
}

void output_word_op(int index)
{
	if (index != -1 && !encountered[index]) // Opcode Table Output
	{
		encountered[index] = true;
		fprintf(op_out, "%-6s %s \n", instrs[index], ops[index]);
	}
}

// --------------------------------------------------     2nd Pass functions  ------------------------------------------------------------------------------

void label_address(char label[], char addr[]) // Returns the address of a label by reading the symTable
{
	char ch;
	int index = 0;
	while((ch = fgetc(sym_out)) != EOF)
	{
		if (ch == ':')
		{
			addr[index] = '\0';
			if (strcmp(addr, label) == 0) // If the word matches the label
			{
				index = 0;
				ch = fgetc(sym_out); // skip the space
				while((ch = fgetc(sym_out)) != '\n') // Take the address of the label
					addr[index++] = ch;
				addr[index] = '\0';
				return;
			}
			else
			{
				while((ch = fgetc(sym_out)) != EOF && ch != '\n'); // Move to the next line
				if (ch == EOF)
					return;
				index = 0;
			}
		}
		else
			addr[index++] = ch;
	}

	// Label not found
	addr[0] = '\0';
	return; 
}

void print_hex(char word[], int mode) // mode 0 for register, 1 for value, 2 for address
{
	if (mode == 0)
	{
		int num = atoi(word);
		char str[6];
		for(int i = 0; i < 5; i++)
		{
			str[4-i] = num % 2 + '0';
			num = num / 2;
		}
		str[5] = '\0';
		fprintf(out, "%s ",str);
	}
	else if (mode == 1)
	{
		char hex_word[4];
		if (word[1] == '\0') // 1 hex digit
		{
			hex_word[0] = '0'; hex_word[1] = '0'; hex_word[2] = '0';
			hex_word[3] = word[0];
		}
		else if (word[2] == '\0') // 2 hex digit
		{
			hex_word[0] = '0'; hex_word[1] = '0'; 
			hex_word[2] = word[0]; hex_word[3] = word[1];
		}
		else if (word[3] == '\0') // 3 hex digit
		{
			hex_word[0] = '0'; 
			hex_word[1] = word[0]; hex_word[2] = word[1]; hex_word[3] = word[2];
		}
		else if (word[4] == '\0') // 4 hex digit
		{
			hex_word[0] = word[0]; hex_word[1] = word[1]; hex_word[2] = word[2]; hex_word[3] = word[3];
		}
		else
		{
			fprintf(out, "\nerror in mode == 1\n");
			exit(0);
		}

		fprintf(out, "%s",hex_digit_to_bin(hex_word[0]));
		fprintf(out, "%s",hex_digit_to_bin(hex_word[1]));
		fprintf(out, " ");
		fprintf(out, "%s",hex_digit_to_bin(hex_word[2]));
		fprintf(out, "%s",hex_digit_to_bin(hex_word[3]));
		fprintf(out, " ");
	}
	else if (mode == 2)
	{
		fprintf(out, "%s",hex_digit_to_bin(word[0]));
		fprintf(out, " ");
		fprintf(out, "%s",hex_digit_to_bin(word[1]));
		fprintf(out, " ");
		fprintf(out, "%s",hex_digit_to_bin(word[2]));
		fprintf(out, " ");
		fprintf(out, "%s",hex_digit_to_bin(word[3]));
		fprintf(out, " ");
	}
	else
		printf("\nError in print_hex\n");
}

char* hex_digit_to_bin(char c) // returns the 4 digit binary string corresponding to a hex digit
{
	if (c == '0') return "0000";
	else if (c == '1') return "0001";
	else if (c == '2') return "0010";
	else if (c == '3') return "0011";
	else if (c == '4') return "0100";
	else if (c == '5') return "0101";
	else if (c == '6') return "0110";
	else if (c == '7') return "0111";
	else if (c == '8') return "1000";
	else if (c == '9') return "1001";
	else if (c == 'A') return "1010";
	else if (c == 'B') return "1011";
	else if (c == 'C') return "1100";
	else if (c == 'D') return "1101";
	else if (c == 'E') return "1110";
	else if (c == 'F') return "1111";
	else printf("Error in hex_digit_to_bin");
}