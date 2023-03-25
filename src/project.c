#include "project.h"
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

/* Macro Definitions */

#define compute_INV(result, val) \
  {                              \
    switch (val)                 \
    {                            \
    case LOGIC_0:                \
      result = LOGIC_1;          \
      break;                     \
    case LOGIC_1:                \
      result = LOGIC_0;          \
      break;                     \
    case LOGIC_X:                \
      result = LOGIC_X;          \
      break;                     \
    default:                     \
      assert(0);                 \
    }                            \
  }

#define compute_AND(result, val1, val2) \
  switch (val1)                         \
  {                                     \
  case LOGIC_0:                         \
    result = LOGIC_0;                   \
    break;                              \
  case LOGIC_1:                         \
    result = val2;                      \
    break;                              \
  case LOGIC_X:                         \
    if (val2 == LOGIC_0)                \
      result = LOGIC_0;                 \
    else                                \
      result = LOGIC_X;                 \
    break;                              \
  default:                              \
    assert(0);                          \
  }

#define compute_OR(result, val1, val2) \
  {                                    \
    switch (val1)                      \
    {                                  \
    case LOGIC_1:                      \
      result = LOGIC_1;                \
      break;                           \
    case LOGIC_0:                      \
      result = val2;                   \
      break;                           \
    case LOGIC_X:                      \
      if (val2 == LOGIC_1)             \
        result = LOGIC_1;              \
      else                             \
        result = LOGIC_X;              \
      break;                           \
    default:                           \
      assert(0);                       \
    }                                  \
  }

#define evaluate(gate)                                           \
  {                                                              \
    switch (gate.type)                                           \
    {                                                            \
    case PI:                                                     \
      break;                                                     \
    case PO:                                                     \
    case BUF:                                                    \
      gate.out_val = gate.in_val[0];                             \
      break;                                                     \
    case PO_GND:                                                 \
      gate.out_val = LOGIC_0;                                    \
      break;                                                     \
    case PO_VCC:                                                 \
      gate.out_val = LOGIC_1;                                    \
      break;                                                     \
    case INV:                                                    \
      compute_INV(gate.out_val, gate.in_val[0]);                 \
      break;                                                     \
    case AND:                                                    \
      compute_AND(gate.out_val, gate.in_val[0], gate.in_val[1]); \
      break;                                                     \
    case NAND:                                                   \
      compute_AND(gate.out_val, gate.in_val[0], gate.in_val[1]); \
      compute_INV(gate.out_val, gate.out_val);                   \
      break;                                                     \
    case OR:                                                     \
      compute_OR(gate.out_val, gate.in_val[0], gate.in_val[1]);  \
      break;                                                     \
    case NOR:                                                    \
      compute_OR(gate.out_val, gate.in_val[0], gate.in_val[1]);  \
      compute_INV(gate.out_val, gate.out_val);                   \
      break;                                                     \
    default:                                                     \
      assert(0);                                                 \
    }                                                            \
  }

#define erase_inputs(circuit, num)              \
  {                                             \
    switch (circuit->gate[num].type)            \
    {                                           \
    /* gates with no input terminal */          \
    case PI:                                    \
    case PO_GND:                                \
    case PO_VCC:                                \
      break;                                    \
    /* gates with one input terminal */         \
    case INV:                                   \
    case BUF:                                   \
    case PO:                                    \
      circuit->gate[num].in_val[0] = UNDEFINED; \
      break;                                    \
    /* gates with two input terminals */        \
    case AND:                                   \
    case NAND:                                  \
    case OR:                                    \
    case NOR:                                   \
      circuit->gate[num].in_val[0] = UNDEFINED; \
      circuit->gate[num].in_val[1] = UNDEFINED; \
      break;                                    \
    default:                                    \
      assert(0);                                \
    }                                           \
  }

/*************************************************************************

Function:  three_val_fault_simulate

Purpose:  This function performs fault simulation on 3-valued input patterns.

pat.out[][] is filled with the fault-free output patterns corresponding to
the input patterns in pat.in[][].

Return:  List of faults that remain undetected.

*************************************************************************/

fault_list_t *three_val_fault_simulate(ckt, pat, undetected_flist)
circuit_t *ckt;
pattern_t *pat;
fault_list_t *undetected_flist;
{
  // printf("start here\n");

  int p;          /* looping variable for pattern number */
  int i;          /* looping variable for gates */
  int j;          /* looping variable for fanout additions/subtractions */
  int k;          /* variable for fanout head of list */
  int l;          /* variable for fanout tail of list */
  int fanout_sum; /* total fanouts in fanout list thus far (l-k+1) */
  fault_list_t *fptr, *prev_fptr;
  int detected_flag;
  int undetectable_flag;                     // fault dissipates within gate; checks with fault-free
  char ckt_inputs[pat->len][ckt->ngates][2]; // copy of all inputs from fault-free circuit
  char ckt_outputs[pat->len][ckt->ngates];   // copy of all outputs from fault-free circuit
  int fanout_list[2 * (ckt->ngates)];       // list of fanouts that need to be checked

  // printf("here0\n");

  /*************************/
  /* fault-free simulation */
  /*************************/

  /* loop through all patterns */
  for (p = 0; p < pat->len; p++)
  {
    /* initialize all gate values to UNDEFINED */
    for (i = 0; i < ckt->ngates; i++)
    {
      ckt->gate[i].in_val[0] = UNDEFINED;
      ckt->gate[i].in_val[1] = UNDEFINED;
      ckt->gate[i].out_val = UNDEFINED;
    }
    /* assign primary input values for pattern */
    for (i = 0; i < ckt->npi; i++)
    {
      ckt->gate[ckt->pi[i]].out_val = pat->in[p][i];
      ckt_outputs[p][i] = pat->in[p][i];
    }
    /* evaluate all gates */
    for (i = 0; i < ckt->ngates; i++)
    {
      /* get gate input values */
      switch (ckt->gate[i].type)
      {
      /* gates with no input terminal */
      case PI:
      case PO_GND:
      case PO_VCC:
        break;
      /* gates with one input terminal */
      case INV:
      case BUF:
      case PO:
        ckt->gate[i].in_val[0] = ckt->gate[ckt->gate[i].fanin[0]].out_val;
        ckt_inputs[p][i][0] = ckt->gate[ckt->gate[i].fanin[0]].out_val;
        break;
      /* gates with two input terminals */
      case AND:
      case NAND:
      case OR:
      case NOR:
        ckt->gate[i].in_val[0] = ckt->gate[ckt->gate[i].fanin[0]].out_val;
        ckt->gate[i].in_val[1] = ckt->gate[ckt->gate[i].fanin[1]].out_val;
        ckt_inputs[p][i][0] = ckt->gate[ckt->gate[i].fanin[0]].out_val;
        ckt_inputs[p][i][1] = ckt->gate[ckt->gate[i].fanin[1]].out_val;
        break;
      default:
        assert(0);
      }
      /* compute gate output value */
      evaluate(ckt->gate[i]);
      ckt_outputs[p][i] = ckt->gate[i].out_val;
    }
    /* put fault-free primary output values into pat data structure */
    for (i = 0; i < ckt->npo; i++)
    {
      pat->out[p][i] = ckt->gate[ckt->po[i]].out_val;
    }
  }

  // printf("here1\n");

  /********************/
  /* fault simulation */
  /********************/

  /* initialize all gate values to UNDEFINED */
  for (i = 0; i < ckt->ngates; i++)
  {
    ckt->gate[i].in_val[0] = UNDEFINED;
    ckt->gate[i].in_val[1] = UNDEFINED;
    ckt->gate[i].out_val = UNDEFINED;
    ckt->gate[i].fault_prone = FALSE;
    ckt->gate[i].fault_prone_num = 0;
    ckt->gate[i].duplicate = FALSE;
  }

  // printf("here2\n");

  /* loop through all undetected faults */
  prev_fptr = (fault_list_t *)NULL;
  for (fptr = undetected_flist; fptr != (fault_list_t *)NULL; fptr = fptr->next)
  {
    /* loop through all patterns */
    detected_flag = FALSE;
    for (p = 0; (p < pat->len); p++)
    {
      undetectable_flag = FALSE;
      fanout_sum = 1;
      k = 0;
      l = 0;
      i = fptr->gate_index;

      // printf("here3\n");

      /* evaluate all gates */
      while (fanout_sum != 0)
      {
        //printf("new gate\n");
        if (ckt->gate[i].duplicate == TRUE)
        {
          ckt->gate[i].duplicate = FALSE;
          k += 1;
          i = fanout_list[k];
          continue;
        }
        /* get the input values for the gate to fault-free or fan-in */
        int input0;
        int input1;
        if (ckt->gate[ckt->gate[i].fanin[0]].out_val == UNDEFINED)
          input0 = ckt_inputs[p][i][0];
        else
          input0 = ckt->gate[ckt->gate[i].fanin[0]].out_val;
        if (ckt->gate[ckt->gate[i].fanin[1]].out_val == UNDEFINED)
          input1 = ckt_inputs[p][i][1];
        else
          input1 = ckt->gate[ckt->gate[i].fanin[1]].out_val;

        /* set gate input values, fault-prone gate, and fault-prone inputs */
        switch (ckt->gate[i].type)
        {
          /* gates with no input terminal */
        case PI:
        case PO_GND:
        case PO_VCC:
          break;
          /* gates with one input terminal */
        case INV:
        case BUF:
        case PO:
          ckt->gate[i].in_val[0] = input0;
          ckt->gate[i].fault_prone = ckt->gate[ckt->gate[i].fanin[0]].fault_prone;
          if (ckt->gate[ckt->gate[i].fanin[0]].fault_prone == TRUE)
            ckt->gate[i].fault_prone_num = 1;
          else
            ckt->gate[i].fault_prone_num = 0;
          break;
          /* gates with two input terminals */
        case AND:
        case NAND:
        case OR:
        case NOR:
          ckt->gate[i].in_val[0] = input0;
          ckt->gate[i].in_val[1] = input1;
          ckt->gate[i].fault_prone = (ckt->gate[ckt->gate[i].fanin[0]].fault_prone ||
                                      ckt->gate[ckt->gate[i].fanin[1]].fault_prone);
          if ((ckt->gate[ckt->gate[i].fanin[0]].fault_prone &
               ckt->gate[ckt->gate[i].fanin[1]].fault_prone) == TRUE)
          {
            ckt->gate[i].fault_prone_num = 2;
            ckt->gate[i].duplicate = TRUE;
          }
          else if ((ckt->gate[ckt->gate[i].fanin[0]].fault_prone ^
                    ckt->gate[ckt->gate[i].fanin[1]].fault_prone) == TRUE)
            ckt->gate[i].fault_prone_num = 1;
          else
            ckt->gate[i].fault_prone_num = 0;
          break;
        default:
          assert(0);
        }

        // printf("here4\n");

        /* check if faulty gate */
        if (i == fptr->gate_index)
        {
          /* check if fault at input */
          if (fptr->input_index >= 0)
          {
            //printf("Fault at input \n");
            /* set fault as input if different from fault-free input */
            /* S_A_0 */
            if ((fptr->type == S_A_0) && (ckt_inputs[p][i][fptr->input_index] != LOGIC_0))
            {
              ckt->gate[i].in_val[fptr->input_index] = LOGIC_0;
              ckt->gate[i].fault_prone = TRUE;
            }
            /* S_A_1 */
            else if ((fptr->type == S_A_1) && (ckt_inputs[p][i][fptr->input_index] != LOGIC_1))
            {
              ckt->gate[i].in_val[fptr->input_index] = LOGIC_1;
              ckt->gate[i].fault_prone = TRUE;
            }
            /* if the fault input is the same as the fault-free input, fault cannot be detected */
            else
            {
              erase_inputs(ckt, i);
              undetectable_flag = TRUE;
              break;
            }
            /* compute gate output value */
            evaluate(ckt->gate[i]);
            //printf("Gate type: %d \n", ckt->gate[i].type);
            //printf("Gate output value: %d\n", ckt->gate[i].out_val);
            //printf("Gate f.f. output value: %d\n", ckt_outputs[p][i]);
            /* if the computed value is the same as fault-free or output
            is a don't care, fault cannot be detected/doesn't matter */
            if ((ckt->gate[i].out_val == ckt_outputs[p][i]) || (ckt_outputs[p][i] == LOGIC_X))
            {
              erase_inputs(ckt, i);
              ckt->gate[i].fault_prone = FALSE;
              ckt->gate[i].fault_prone_num = 0;
              ckt->gate[i].out_val = UNDEFINED;
              undetectable_flag = TRUE;
              break;
            }
            /* if the computed value is a primary output, and that primary output is different
            than the fault-free primary output, the fault can be detected */
            else if ((ckt->gate[i].type == PO) && (ckt->gate[i].out_val != ckt_outputs[p][i]))
            {
              erase_inputs(ckt, i);
              ckt->gate[i].fault_prone = FALSE;
              ckt->gate[i].fault_prone_num = 0;
              ckt->gate[i].out_val = UNDEFINED;
              detected_flag = TRUE;
              break;
            }
            else if ((ckt->gate[i].type == PO) && (ckt->gate[i].out_val != ckt_outputs[p][i]))
            {
              erase_inputs(ckt, i);
              ckt->gate[i].fault_prone = FALSE;
              ckt->gate[i].fault_prone_num = 0;
              ckt->gate[i].out_val = UNDEFINED;
              detected_flag = TRUE;
              break;
            }
            /* if the computed value is not the same and not
            a primary output, fault needs to be propagated */
            else
            {
              for (j = 0; j < ckt->gate[i].num_fanout; j++)
                fanout_list[j] = ckt->gate[i].fanout[j];
              fanout_sum = ckt->gate[i].num_fanout;
              l = fanout_sum - 1;
              //printf("**Fault present \n");
              //printf("Number of fanouts for gate: %d\n", ckt->gate[i].num_fanout);
              //printf("%d\n", i);
              // printf("%d\n", ckt->po[i]);
              // printf("%d\n", ckt->gate[i].num_fanout);
              // printf("%d\n", ckt->gate[i].type);
              // printf("%d\n%d\n", ckt->gate[i].out_val, ckt_outputs[p][i]);
              //printf("first fault in list: %d \n", fanout_list[0]);
              //printf("first fault in gate: %d \n", ckt->gate[i].fanout[0]);
              i = ckt->gate[i].fanout[0];
              //printf("%d\n", i);
            }
            // printf("here5\n");
            // printf("%d\n", i);
            // assert(0);
          }
          /* fault at output */
          else
          {
            //printf("Fault at output \n");
            evaluate(ckt->gate[i]);
            // printf("%d\n", ckt->gate[i].type);
            // printf("%d\n", ckt->gate[i].out_val);
            // printf("%d\n", ckt_outputs[p][i]);
            /* set fault as output if different from fault-free output */
            /* S_A_0 */
            //printf("Gate type: %d \n", ckt->gate[i].type);
            //printf("Gate output value: %d\n", ckt->gate[i].out_val);
            //printf("Gate f.f. output value: %d\n", ckt_outputs[p][i]);
            //printf("Fanout sum: %d \n", fanout_sum);
            if ((fptr->type == S_A_0) && (ckt_outputs[p][i] != LOGIC_0))
            {
              //printf("**Fault present");
              ckt->gate[i].out_val = LOGIC_0;
              ckt->gate[i].fault_prone = TRUE;
              //printf("Number of fanouts for gate: %d\n", ckt->gate[i].num_fanout);
              for (j = 0; j < ckt->gate[i].num_fanout; j++)
                fanout_list[j] = ckt->gate[i].fanout[j];
              //printf("first fault in list: %d \n", fanout_list[0]);
              //printf("first fault in gate: %d \n", ckt->gate[i].fanout[0]);
              fanout_sum = ckt->gate[i].num_fanout;
              l = fanout_sum - 1;
              i = ckt->gate[i].fanout[0];
            }
            /* S_A_1 */
            else if ((fptr->type == S_A_1) && (ckt_outputs[p][i] != LOGIC_1))
            {
              //printf("**Fault present");
              ckt->gate[i].out_val = LOGIC_1;
              ckt->gate[i].fault_prone = TRUE;
              //printf("Number of fanouts for gate: %d\n", ckt->gate[i].num_fanout);
              for (j = 0; j < ckt->gate[i].num_fanout; j++)
                fanout_list[j] = ckt->gate[i].fanout[j];
              //printf("first fault in list: %d \n", fanout_list[0]);
              //printf("first fault in gate: %d \n", ckt->gate[i].fanout[0]);
              fanout_sum = ckt->gate[i].num_fanout;
              l = fanout_sum - 1;
              i = ckt->gate[i].fanout[0];
            }
            /* if the computed value is the same as fault-free or output
            is a don't care, fault cannot be detected/doesn't matter */
            if ((ckt->gate[i].out_val == ckt_outputs[p][i]) || (ckt_outputs[p][i] == LOGIC_X))
            {
              erase_inputs(ckt, i);
              ckt->gate[i].fault_prone = FALSE;
              ckt->gate[i].fault_prone_num = 0;
              ckt->gate[i].out_val = UNDEFINED;
              undetectable_flag = TRUE;
              break;
            }
            /* if the output of the gate is a primary output, and that primary output is different
            than the fault-free primary output, the fault can be detected */
            else if ((ckt->gate[i].type == PO) && (ckt->gate[i].out_val != ckt_outputs[p][i]))
            {
              erase_inputs(ckt, i);
              ckt->gate[i].fault_prone = FALSE;
              ckt->gate[i].fault_prone_num = 0;
              ckt->gate[i].out_val = UNDEFINED;
              undetectable_flag = TRUE;
              break;
            }
            else if ((ckt->gate[i].type == PO) && (ckt->gate[i].out_val != ckt_outputs[p][i]))
            {
              erase_inputs(ckt, i);
              ckt->gate[i].fault_prone = FALSE;
              ckt->gate[i].fault_prone_num = 0;
              ckt->gate[i].out_val = UNDEFINED;
              undetectable_flag = TRUE;
              break;
            }
            /* if the fault output is the same as the fault-free output, fault cannot be detected */
            else
            {
              ckt->gate[i].out_val = UNDEFINED;
              undetectable_flag = TRUE;
              break;
            }
          }
          // printf("%d", ckt->gate[i].out_val);
          // printf("%c", ckt->gate[i].fault_prone);
          // assert(0);
        }
        else
        { /* not fault injection gate */
          /* compute gate output value */
          //printf("Fault propagated gate \n");
          evaluate(ckt->gate[i]);
          //printf("Gate type: %d \n", ckt->gate[i].type);
          //printf("Gate output value: %d\n", ckt->gate[i].out_val);
          //printf("Gate f.f. output value: %d\n", ckt_outputs[p][i]);
          //printf("Fanout sum: %d \n", fanout_sum);
          /* if the fault dissipates, substract from fanout_sum by number of prone inputs */
          if ((ckt->gate[i].out_val == ckt_outputs[p][i]) && (ckt->gate[i].fault_prone == TRUE))
          {
            if (ckt->gate[i].fault_prone_num == 2)
              fanout_sum -= 2;
            else
              fanout_sum -= 1;
            ckt->gate[i].fault_prone_num = 0;
            ckt->gate[i].fault_prone == FALSE;
            erase_inputs(ckt, i);
            ckt->gate[i].out_val = UNDEFINED;
            k += 1;
            i = fanout_list[k];
          }
          /* if the computed value is the same as fault-free or output
          is a don't care, fault cannot be detected/doesn't matter */
          if ((ckt->gate[i].out_val == ckt_outputs[p][i]) || (ckt_outputs[p][i] == LOGIC_X))
          {
            erase_inputs(ckt, i);
            ckt->gate[i].fault_prone = FALSE;
            ckt->gate[i].fault_prone_num = 0;
            ckt->gate[i].out_val = UNDEFINED;
            undetectable_flag = TRUE;
            break;
          }
          /* if the output of the gate is a primary output, and that primary output is different
          than the fault-free primary output, the fault can be detected */
          else if ((ckt->gate[i].type == PO) && (ckt->gate[i].out_val != ckt_outputs[p][i]))
          {
            erase_inputs(ckt, i);
            ckt->gate[i].fault_prone = FALSE;
            ckt->gate[i].fault_prone_num = 0;
            ckt->gate[i].out_val = UNDEFINED;
            undetectable_flag = TRUE;
            break;
          }
          else if ((ckt->gate[i].type == PO) && (ckt->gate[i].out_val != ckt_outputs[p][i]))
          {
            erase_inputs(ckt, i);
            ckt->gate[i].fault_prone = FALSE;
            ckt->gate[i].fault_prone_num = 0;
            ckt->gate[i].out_val = UNDEFINED;
            undetectable_flag = TRUE;
            break;
          }
          /* if the fault doesn't dissipate, continue with propagation */
          else
          {
            if (ckt->gate[i].fault_prone_num == 2)
              fanout_sum = (fanout_sum + ckt->gate[i].num_fanout - 2);
            else
              fanout_sum = (fanout_sum + ckt->gate[i].num_fanout - 1);

            for (j = 0; j < ckt->gate[i].num_fanout; j++)
              fanout_list[j + l + 1] = ckt->gate[i].fanout[j];

            l += fanout_sum;
            ckt->gate[i].fault_prone_num = 0;
            ckt->gate[i].fault_prone == FALSE;
            erase_inputs(ckt, i);
            k += 1;
            i = fanout_list[k];
          }
        }
      }

      //printf("next pattern\n");

      /* if the fault is detected, break out. Otherwise, continue with pattern inputs */
      if (detected_flag == TRUE)
        break;
    }
    if (detected_flag)
    {
      /* remove fault from undetected fault list */
      if (prev_fptr == (fault_list_t *)NULL)
      {
        /* if first fault in fault list, advance head of list pointer */
        undetected_flist = fptr->next;
      }
      else
      { /* if not first fault in fault list, then remove link */
        prev_fptr->next = fptr->next;
      }
    }
    else
    { /* fault remains undetected, keep on list */
      prev_fptr = fptr;
    }
  }
  return (undetected_flist);
}
