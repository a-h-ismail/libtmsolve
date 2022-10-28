/*
Copyright (C) 2021-2022 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#include "scientific.h"
#include "internals.h"
#include "function.h"
#include "string_tools.h"
char *function_name[] =
    {"fact", "abs", "ceil", "floor", "acosh", "asinh", "atanh", "acos", "asin", "atan", "cosh", "sinh", "tanh", "cos", "sin", "tan", "ln", "log"};
double (*math_function[])(double) =
    {factorial, fabs, ceil, floor, acosh, asinh, atanh, acos, asin, atan, cosh, sinh, tanh, cos, sin, tan, log, log10};
// Extended functions, may take more than one parameter (stored in a comma separated string)
char *ext_function_name[] = {"int", "der"};
double (*ext_math_function[])(char *) =
    {integral_processor, derivative};
// Complex functions
char *cmplx_function_name[] =
    {"abs", "arg", "acosh", "asinh", "atanh", "acos", "asin", "atan", "cosh", "sinh", "tanh", "cos", "sin", "tan", "log"};
double complex (*cmplx_function[])(double complex) =
    {cabs, carg, cacosh, casinh, catanh, cacos, casin, catan, ccosh, csinh, ctanh, ccos, csin, ctan, clog};

double factorial(double value)
{
    double result = 1, i;
    for (i = 2; i <= value; ++i)
        result *= i;
    return result;
}
double calculate_expr(char *expr)
{
    double result;
    s_expression *subexp;
    // Check for empty input
    if (expr[0] == '\0')
    {
        error_handler("Empty input.", 1, 1, -1);
        return false;
    }
    // Combine multiple add/subtract symbols (ex: -- becomes + or +++++ becomes +)
    combine_add_subtract(expr, 0, strlen(expr) - 1);
    if (parenthesis_check(expr) == false)
        return false;
    if (implicit_multiplication(expr) == false)
        return false;
    subexp = parse_expr(expr, false, false);
    if (subexp == NULL)
        return NAN;
    result = evaluate(subexp);
    delete_subexps(subexp);
    return result;
}
// Evaluates the expresssion
double complex evaluate(math_expr *math_struct)
{
    node *i_node;
    int subexpr_index = 0, subexpr_count = math_struct->subexpr_count;
    s_expression *subexpr_ptr;
    double complex result;
    /*
    #ifdef DEBUG
        // Dumping node data
        do
        {
            if (i_node == NULL)
            {
                puts("Special function");
            }
            else
            {
                i_node = subexpr_ptr[subexpr_index].node_list + subexpr_ptr[subexpr_index].start_node;
                while (i_node != NULL)
                {
                    printf("%.8g %c %.8g -> ", (double)i_node->left_operand, i_node->operator, (double)i_node->right_operand);
                    i_node = i_node->next;
                }
                printf("Function: %lli", subexpr_ptr[subexpr_index].function_ptr);
                puts("|");
            }
            ++subexpr_index;
        } while (subexpr_ptr[subexpr_index - 1].last_subexp == false);
        puts("End");
        fflush(stdout);
    #endif
    */
    while (subexpr_index < subexpr_count)
    {
        if (subexpr_ptr[subexpr_index].node_list == NULL)
        {
            char *args;
            int length = subexpr_ptr[subexpr_index].solve_end - subexpr_ptr[subexpr_index].solve_start + 1;
            args = malloc((length + 1) * sizeof(char));
            memcpy(args, g_exp + subexpr_ptr[subexpr_index].solve_start, length);
            args[length] = '\0';
            **(subexpr_ptr[subexpr_index].result) = (*(subexpr_ptr[subexpr_index].ext_function_ptr))(args);
            if (isnan((double)**(subexpr_ptr[subexpr_index].result)))
            {
                error_handler("Math error", 1, 0, subexpr_ptr[subexpr_index].solve_start);
                return NAN;
            }
            ++subexpr_index;
            free(args);
            continue;
        }
        i_node = subexpr_ptr[subexpr_index].node_list + subexpr_ptr[subexpr_index].start_node;

        if (subexpr_ptr[subexpr_index].op_count == 0)
            *(i_node->node_result) = i_node->left_operand;
        else
            while (i_node != NULL)
            {
                switch (i_node->operator)
                {
                case '+':
                    *(i_node->node_result) = i_node->left_operand + i_node->right_operand;
                    break;

                case '-':
                    *(i_node->node_result) = i_node->left_operand - i_node->right_operand;
                    break;

                case '*':
                    *(i_node->node_result) = i_node->left_operand * i_node->right_operand;
                    break;

                case '/':
                    if (i_node->right_operand == 0)
                    {
                        error_handler("Error: Division by zero", 1, 1, i_node->operator_index);
                        return NAN;
                    }
                    *(i_node->node_result) = i_node->left_operand / i_node->right_operand;
                    break;

                case '%':
                    if (i_node->right_operand == 0)
                    {
                        error_handler("Error: Modulo zero implies a Division by 0.", 1, 1, i_node->operator_index);
                        return NAN;
                    }
                    *(i_node->node_result) = fmod(i_node->left_operand, i_node->right_operand);
                    break;

                case '^':
                    *(i_node->node_result) = pow(i_node->left_operand, i_node->right_operand);
                    break;
                }
                i_node = i_node->next;
            }
        // Executing function on the subexpression result
        if (subexpr_ptr[subexpr_index].function_ptr != NULL)
        {
            **(subexpr_ptr[subexpr_index].result) = (*(subexpr_ptr[subexpr_index].function_ptr))(**(subexpr_ptr[subexpr_index].result));
            if (isnan((double)**(subexpr_ptr[subexpr_index].result)))
            {
                error_handler("Math error", 1, 0, subexpr_ptr[subexpr_index].solve_start);
                return NAN;
            }
        }
        ++subexpr_index;
    }
    return result;
}
// Sets the variable in x_node to the left or right operand "r","l"
// Returns true if the variable x was found, false otherwise
bool set_variable_in_node(char *exp, node *x_node, char operand)
{
    bool is_negative = false, is_variable = false;
    if (*exp == 'x')
        is_variable = true;
    else if (exp[1] == 'x')
    {
        if (*exp == '+')
            is_variable = true;
        else if (*exp == '-')
            is_variable = is_negative = true;
    }
    // The value is not the variable x
    else
        return false;
    if (is_variable)
    {
        // x as left op
        if (operand == 'l')
        {
            if (is_negative)
                x_node->variable_operands = x_node->variable_operands | 0b101;
            else
                x_node->variable_operands = x_node->variable_operands | 0b1;
        }
        // x as right op
        else if (operand == 'r')
        {
            if (is_negative)
                x_node->variable_operands = x_node->variable_operands | 0b1010;
            else
                x_node->variable_operands = x_node->variable_operands | 0b10;
        }
        else
        {
            puts("Invaild operand argument.");
            exit(2);
        }
    }
    return true;
}
// Parse a math expression into a s_expression array
math_expr *parse_expr(char *expr, bool enable_variables, bool enable_complex)
{
    int i, j, length, subexpr_count = 1;
    int subexpr_index;
    int buffer_size = 10, buffer_step = 10;
    length = strlen(expr);
    s_expression *subexps_ptr;
    node *node_block, *tmp_node;
    math_expr *math_struct = malloc(sizeof(math_expr));
    math_struct->var_count = 0;
    math_struct->variable_ptr = NULL;

    /*
    Parsing steps:
    - Locate and determine the depth of each subexpression (the whole expression has a depth of 0)
    - Fill the expression data (start/end...)
    - Sort subexpressions by depth (high to low)
    - Parse subexpressions from high to low depth
    */

    length = strlen(expr);
    subexps_ptr = malloc(buffer_size * sizeof(s_expression));

    int depth = 0;
    subexpr_index = 0;
    // Determining the depth and start/end of each subexpression parenthesis
    for (i = 0; i < length; ++i)
    {
        // Resize the subexp array on the fly
        if (subexpr_index == buffer_size)
        {
            buffer_size += buffer_step;
            subexps_ptr = realloc(subexps_ptr, buffer_size * sizeof(s_expression));
        }
        if (expr[i] == '(')
        {
            subexps_ptr[subexpr_index].ext_function_ptr = NULL;
            // Treat extended functions as a subexpression
            for (j = 0; j < sizeof(ext_math_function) / sizeof(*ext_math_function); ++j)
            {
                int temp;
                temp = r_search(expr, ext_function_name[j], i - 1, true);
                if (temp != -1)
                {
                    subexps_ptr[subexpr_index].expression_start = temp;
                    subexps_ptr[subexpr_index].solve_start = i + 1;
                    i = find_closing_parenthesis(expr, i);
                    subexps_ptr[subexpr_index].solve_end = i - 1;
                    subexps_ptr[subexpr_index].depth = depth + 1;
                    subexps_ptr[subexpr_index].ext_function_ptr = ext_math_function[j];
                    break;
                }
            }
            if (subexps_ptr[subexpr_index].ext_function_ptr != NULL)
            {
                ++subexpr_index;
                continue;
            }
            // Normal case
            ++depth;
            subexps_ptr[subexpr_index].solve_start = i + 1;
            subexps_ptr[subexpr_index].depth = depth;
            // The expression start is the parenthesis, may change if a function is found
            subexps_ptr[subexpr_index].expression_start = i;
            subexps_ptr[subexpr_index].solve_end = find_closing_parenthesis(expr, i) - 1;
            ++subexpr_index;
        }
        else if (expr[i] == ')')
            --depth;
    }
    // + 1 for the subexpression with depth 0
    subexpr_count = subexpr_index + 1;
    // Shrink the block to the required size
    subexps_ptr = realloc(subexps_ptr, subexpr_count * sizeof(subexpr_count));
    // Copy the pointer to the structure
    math_struct->subexps = subexps_ptr;

    math_struct->subexpr_count = subexpr_count;
    for (i = 0; i < subexpr_count; ++i)
        subexps_ptr[i].function_ptr = subexps_ptr[i].cmplx_function_ptr = NULL;

    // The whole expression "subexpression"
    subexps_ptr[subexpr_index].depth = 0;
    subexps_ptr[subexpr_index].solve_start = subexps_ptr[subexpr_index].expression_start = 0;
    subexps_ptr[subexpr_index].solve_end = length - 1;
    subexps_ptr[subexpr_index].ext_function_ptr = NULL;
    subexps_ptr[subexpr_index].result = &(math_struct->answer);

    // Sort by depth (high to low)
    qsort(subexps_ptr, subexpr_count, sizeof(s_expression), compare_subexps_depth);

    int *operator_index, solve_start, solve_end, op_count, status;
    buffer_size = buffer_step = 1000;

    /*
    How to parse a subexp:
    - Handle the special case of extended functions (example: int, der)
    - Create an array that determines the location of operators in the string
    - Check if the subexpression has a function to execute on the result, set pointer
    - Allocate the array of nodes
    - Use the operator index array to fill the nodes data on operators type and location
    - Fill nodes operator priority
    - Fill nodes with values or set as variable (using x)
    - Set nodes order of calculation (using *next)
    - Set the result pointer of each node relying on its position and neighbor priorities
    - Set the subexp result double pointer to the result pointer of the last calculated node
    */
    for (subexpr_index = 0; subexpr_index < subexpr_count; ++subexpr_index)
    {
        if (subexps_ptr[subexpr_index].ext_function_ptr != NULL)
        {
            subexps_ptr[subexpr_index].node_list = NULL;
            subexps_ptr[subexpr_index].result = malloc(sizeof(double complex **));
            continue;
        }

        // For simplicity
        solve_start = subexps_ptr[subexpr_index].solve_start;
        solve_end = subexps_ptr[subexpr_index].solve_end;

        operator_index = (int *)malloc(buffer_size * sizeof(int));
        // Count number of operators and store it's indexes
        for (i = solve_start, op_count = 0; i <= solve_end; ++i)
        {
            // Skipping over an already processed expression
            if (expr[i] == '(')
            {
                int previous_subexp;
                previous_subexp = subexp_start_at(subexps_ptr, i + 1, subexpr_index, 2);
                if (previous_subexp != -1)
                    i = subexps_ptr[previous_subexp].solve_end + 1;
            }
            else if (is_number(expr[i]))
                continue;

            else if (is_op(*(expr + i)))
            {
                // Skipping a + or - used in scientific notation (like 1e+5)
                if ((expr[i - 1] == 'e' || expr[i - 1] == 'E') && (expr[i] == '+' || expr[i] == '-'))
                {
                    ++i;
                    continue;
                }
                // Varying the array size on demand
                if (op_count == buffer_size)
                {
                    buffer_size += buffer_step;
                    operator_index = realloc(operator_index, buffer_size * sizeof(int));
                }
                operator_index[op_count] = i;
                // Skipping a + or - coming just after an operator
                if (expr[i + 1] == '-' || expr[i + 1] == '+')
                    ++i;
                ++op_count;
            }
        }

        // Searching for any function preceding the expression to set the function pointer
        if (solve_start > 1)
        {
            if (!enable_complex)
            {
                // Determining the number of functions to check at runtime
                int total = sizeof(math_function) / sizeof(*math_function);
                for (i = 0; i < total; ++i)
                {
                    j = r_search(expr, function_name[i], solve_start - 2, true);
                    if (j != -1)
                    {
                        subexps_ptr[subexpr_index].function_ptr = math_function[i];
                        // Setting the start of the subexpression to the start of the function name
                        subexps_ptr[subexpr_index].expression_start = j;
                        break;
                    }
                }
            }
            else
            {
                // Determining the number of complex functions to check at runtime
                int total = sizeof(cmplx_function) / sizeof(*cmplx_function);
                for (i = 0; i < total; ++i)
                {
                    j = r_search(expr, cmplx_function_name[i], solve_start - 2, true);
                    if (j != -1)
                    {
                        subexps_ptr[subexpr_index].cmplx_function_ptr = cmplx_function[i];
                        subexps_ptr[subexpr_index].expression_start = j;
                        break;
                    }
                }
            }
        }

        subexps_ptr[subexpr_index].op_count = op_count;
        // Allocating the array of nodes
        // Case of no operators, create a dummy node and store the value in op1
        if (op_count == 0)
            subexps_ptr[subexpr_index].node_list = malloc(sizeof(node));
        else
            subexps_ptr[subexpr_index].node_list = malloc(op_count * sizeof(node));

        node_block = subexps_ptr[subexpr_index].node_list;

        // Checking if the expression is terminated with an operator
        if (op_count != 0 && operator_index[op_count - 1] == solve_end)
        {
            error_handler("Missing right operand", 1, 1, operator_index[op_count - 1]);
            delete_subexps(subexps_ptr);
            return NULL;
        }
        // Filling operations and index data into each node
        for (i = 0; i < op_count; ++i)
        {
            node_block[i].operator_index = operator_index[i];
            node_block[i].operator= expr[operator_index[i]];
        }
        free(operator_index);

        // Case of expression with one term, use one node with operand1 to hold the number
        if (op_count == 0)
        {
            i = subexp_start_at(subexps_ptr, subexps_ptr[subexpr_index].solve_start, subexpr_index, 1);
            subexps_ptr[subexpr_index].node_list[0].variable_operands = 0;
            // Case of nested no operators expressions, set the result of the deeper expression as the left op of the dummy
            if (i != -1)
                *(subexps_ptr[i].result) = &(node_block->left_operand);
            else
            {
                node_block->left_operand = read_value(expr, solve_start,enable_complex);
                if (isnan((double)node_block->left_operand))
                {
                    status = set_variable_in_node(expr + solve_start, node_block, 'l');
                    if (!status)
                    {
                        error_handler("Syntax error.", 1, 1, solve_start);
                        delete_subexps(math_struct);
                        return NULL;
                    }
                }
            }

            subexps_ptr[subexpr_index].start_node = 0;
            subexps_ptr[subexpr_index].result = &(node_block->node_result);
            node_block[0].next = NULL;
            continue;
        }
        else
        {
            // Filling each node's priority data
            priority_fill(node_block, op_count);
            // Filling nodes with operands, filling operands index and setting variable_operands to 0
            for (i = 0; i < op_count; ++i)
            {
                node_block[i].node_index = i;
                node_block[i].variable_operands = 0;
            }
        }

        // Trying to read first number into first node
        // If a - or + sign is in the beginning of the expression. it will be treated as 0-val or 0+val
        if (expr[solve_start] == '-' || expr[solve_start] == '+')
            node_block[0].left_operand = 0;
        else
        {
            node_block[0].left_operand = read_value(expr, solve_start,enable_complex);
            if (isnan((double)node_block[0].left_operand))
            {
                // Checking for the variable 'x'
                if (enable_variables == true)
                    status = set_variable_in_node(expr + solve_start, node_block, 'l');
                else
                    status = false;
                // Variable x not found, try to read number
                if (!status)
                {
                    status = subexp_start_at(subexps_ptr, solve_start, subexpr_index, 1);
                    if (status == -1)
                    {
                        error_handler("Syntax error.", 1, 1, solve_start);
                        delete_subexps(math_struct);
                        return NULL;
                    }
                    else
                        *(subexps_ptr[status].result) = &(node_block[0].left_operand);
                }
            }
        }

        // Intermediate nodes, read number to the appropriate node operand
        for (i = 0; i < op_count - 1; ++i)
        {
            if (node_block[i].priority >= node_block[i + 1].priority)
            {
                node_block[i].right_operand = read_value(expr, node_block[i].operator_index + 1,enable_complex);
                // Case of reading the number to the right operand of a node
                if (isnan((double)node_block[i].right_operand))
                {
                    // Checking for the variable 'x'
                    if (enable_variables == true)
                        status = set_variable_in_node(expr + node_block[i].operator_index + 1, node_block + i, 'r');
                    else
                        status = false;
                    if (!status)
                    {
                        // Case of a subexpression result as right_operand, set result pointer to right_operand
                        status = subexp_start_at(subexps_ptr, node_block[i].operator_index + 1, subexpr_index, 1);
                        if (status == -1)
                        {
                            error_handler("Syntax error.", 1, 1, node_block[i].operator_index + 1);
                            delete_subexps(math_struct);
                            return NULL;
                        }
                        else
                            *(subexps_ptr[status].result) = &(node_block[i].right_operand);
                    }
                }
            }
            // Case of the node[i] having less priority than the next node, use next node's left_operand
            else
            {
                // Read number
                node_block[i + 1].left_operand = read_value(expr, node_block[i].operator_index + 1,enable_complex);
                if (isnan((double)node_block[i + 1].left_operand))
                {
                    // Checking for the variable 'x'
                    if (enable_variables == true)
                        status = set_variable_in_node(expr + node_block[i].operator_index + 1, node_block + i + 1, 'l');
                    else
                        status = false;
                    if (!status)
                    {
                        status = subexp_start_at(subexps_ptr, node_block[i].operator_index + 1, subexpr_index, 1);
                        if (status == -1)
                        {
                            error_handler("Syntax error.", 1, 1, node_block[i].operator_index + 1);
                            delete_subexps(math_struct);
                            return NULL;
                        }
                        else
                            *(subexps_ptr[status].result) = &(node_block[i + 1].left_operand);
                    }
                }
            }
        }
        // Placing the last number in the last node
        node_block[op_count - 1].right_operand = read_value(expr, node_block[op_count - 1].operator_index + 1,enable_complex);
        if (isnan((double)node_block[op_count - 1].right_operand))
        {
            // Checking for the variable 'x'
            if (enable_variables == true)
                status = set_variable_in_node(expr + node_block[op_count - 1].operator_index + 1, node_block + op_count - 1, 'r');
            else
                status = false;
            if (!status)
            {
                // If an expression is the last term, find it and set the pointers
                status = subexp_start_at(subexps_ptr, node_block[op_count - 1].operator_index + 1, subexpr_index, 1);
                if (status == -1)
                {
                    error_handler("Syntax error.", 1, 1, node_block[i].operator_index + 1);
                    return NULL;
                }
                *(subexps_ptr[status].result) = &(node_block[op_count - 1].right_operand);
            }
        }
        // Set the starting node by searching the first node with the highest priority
        for (i = 3; i > 0; --i)
        {
            for (j = 0; j < op_count; ++j)
            {
                if (node_block[j].priority == i)
                {
                    subexps_ptr[subexpr_index].start_node = j;
                    // Making the main loop exit by setting i outside continue condition
                    i = -1;
                    break;
                }
            }
        }

        // Setting nodes order of execution
        i = subexps_ptr[subexpr_index].start_node;
        int target_priority = node_block[i].priority;
        j = i + 1;
        while (target_priority > 0)
        {
            // Running through the nodes to find a node with the target priority
            while (j < op_count)
            {
                if (node_block[j].priority == target_priority)
                {
                    node_block[i].next = node_block + j;
                    // The node found is used as te start node for the next run
                    i = j;
                }
                ++j;
            }
            --target_priority;
            j = 0;
        }
        // Setting the next pointer of the last node to NULL
        node_block[i].next = NULL;

        // Set result pointers for each node based on position and priority
        tmp_node = &(node_block[subexps_ptr[subexpr_index].start_node]);
        int l_node, r_node;
        while (tmp_node->next != NULL)
        {
            i = tmp_node->node_index;
            // Finding the previous and next nodes to compare
            l_node = i - 1, r_node = i + 1;

            // Optimizing contiguous block of same priority by using previous results (TODO)
            while (l_node != -1)
            {
                if (tmp_node->priority <= node_block[l_node].priority)
                    --l_node;
                else
                    break;
            }

            while (r_node < op_count)
            {
                if (tmp_node->priority < node_block[r_node].priority)
                    ++r_node;
                else
                    break;
            }
            // Case of the first node or a node with no left candidate
            if (l_node == -1)
                tmp_node->node_result = &(node_block[r_node].left_operand);
            // Case of a node between 2 nodes
            else if (l_node > -1 && r_node < op_count)
            {
                if (node_block[l_node].priority >= node_block[r_node].priority)
                    tmp_node->node_result = &(node_block[l_node].right_operand);
                else
                    tmp_node->node_result = &(node_block[r_node].left_operand);
            }
            // Case of the last node
            else if (i == op_count - 1)
                tmp_node->node_result = &(node_block[l_node].right_operand);
            tmp_node = tmp_node->next;
        }
        // Case of the last node in the traversal order, set result to be result of the subexpression
        subexps_ptr[subexpr_index].result = &(tmp_node->node_result);
    }

    return subexps_ptr;
}
// Free a subexpressions array and its contents
void delete_subexps(math_expr *math_struct)
{
    int i = 0, count;
    s_expression *subexps = math_struct->subexps;
    for (i = 0; i < math_struct->subexpr_count; ++i)
    {
        if (subexps[i].ext_function_ptr != NULL)
            free(subexps[i].result);
        free(subexps[i].node_list);
    }
    free(subexps);
    free(math_struct->variable_ptr);
    free(math_struct);
}

// Function that factorizes a 32 bit number, returns factors in a heap allocated array
// | factor0 | occurences_of_factor0 | factor1 | occurences_of_factor1 |
int32_t *find_factors(int32_t number)
{
    int32_t dividend = 2, *factors;
    int i = 0;
    // factors are stored in an 64 element array
    //*(factors+2*x) -> factors
    //*(factors+2*x+1) -> number of occurences of the factor *(factors+2*x)
    factors = (int32_t *)calloc(64, sizeof(int));
    if (number == 0)
        return factors;
    *(factors) = 1;
    *(factors + 1) = 1;
    // Turning negative numbers into positive for factorization to work
    if (number < 0)
        number = -number;
    // Simple factorization algorithm
    while (number != 1)
    {
        if (number % dividend == 0)
        {
            ++i;
            number = number / dividend;
            *(factors + (i * 2)) = dividend;
            *(factors + (i * 2) + 1) = 1;
            while (number % dividend == 0 && number >= dividend)
            {
                number = number / dividend;
                ++*(factors + (i * 2) + 1);
            }
        }
        else
        {
            // Optimize by skipping numbers greater than number/2 (in other terms, number itself is number prime)
            if (dividend > number / 2 && number != 1)
            {
                ++i;
                *(factors + (i * 2)) = number;
                *(factors + (i * 2) + 1) = 1;
                break;
            }
        }
        // Optimizing factorization by skipping even numbers (> 2)
        if (dividend > 2)
            dividend = dividend + 2;
        else
            ++dividend;
    }
    if (i == 0)
    {
        *(factors + 2) = number;
        *(factors + 3) = 1;
    }
    return factors;
}

void fraction_reducer(int32_t *num_factors, int32_t *denom_factors, int32_t *numerator, int32_t *denominator)
{
    int i = 1, j = 1, min;
    while (*(num_factors + 2 * i) != 0 && *(denom_factors + 2 * j) != 0)
    {
        if (*(num_factors + 2 * i) == *(denom_factors + 2 * j))
        {
            // upon finding the minimun power we subtract it from the power of the num and denum
            min = find_min(*(num_factors + (2 * i) + 1), *(denom_factors + (2 * j) + 1));
            *(num_factors + (2 * i) + 1) -= min;
            *(denom_factors + (2 * j) + 1) -= min;
            ++i;
            ++j;
        }
        else
        {
            // If the factor at i is smaller increment i , otherwise increment j
            if (*(num_factors + (2 * i)) < *(denom_factors + (2 * j)))
                ++i;
            else
                ++j;
        }
    }
    *numerator = 1;
    *denominator = 1;
    // Calculate numerator and denominator after reduction
    for (i = 1; *(num_factors + (2 * i)) != 0; ++i)
    {
        if (*(num_factors + (2 * i) + 1) != 0)
            *numerator *= pow(*(num_factors + (2 * i)), *(num_factors + (2 * i) + 1));
    }
    for (i = 1; *(denom_factors + (2 * i)) != 0; ++i)
    {
        if (*(denom_factors + (2 * i) + 1) != 0)
            *denominator *= pow(*(denom_factors + (2 * i)), *(denom_factors + (2 * i) + 1));
    }
}

void fraction_processor(char *exp, bool wasdecimal)
{
    int success_count;
    int32_t v1, v2, v, r;
    double check1, check2;
    success_count = sscanf(exp, "%lf/%lf", &check1, &check2);
    if (success_count != 2)
    {
        puts("Syntax error.");
        return;
    }
    // Given that fraction reducer operates on positive numbers, abs(int32_t_MIN)>2147483647 can't be accepted
    if (fabs(check1) > 2147483647 || fabs(check2) > 2147483647)
    {
        puts("Values must be in range [-2147483647,2147483647]");
        return;
    }
    v1 = check1;
    v2 = check2;
    v = v1 / v2;
    r = v1 % v2;
    if (r == 0)
    {
        printf("%s = %" PRId32 "", exp, v);
        if (wasdecimal == false)
            printf("\n");
        return;
    }
    if ((r != 0 || v1 < v2) && v1 != 0)
    {
        int32_t *f_v1, *f_v2;
        // flip signs of v1 and v2 if both are negative or if v2 only is negative to move the - to v1
        if ((v1 < 0 && v2 < 0) || (v1 > 0 && v2 < 0))
        {
            v1 = -v1;
            v2 = -v2;
        }
        if (v1 > v2)
            v1 = v1 % v2;
        if (v1 < 0)
        {
            v1 = -v1;
            v1 = v1 % v2;
            v1 = -v1;
        }
        f_v1 = find_factors(v1);
        f_v2 = find_factors(v2);
        if (v1 < 0)
        {
            v1 = -v1;
            fraction_reducer(f_v1, f_v2, &v1, &v2);
            v1 = -v1;
        }
        else
            fraction_reducer(f_v1, f_v2, &v1, &v2);
        if (v != 0)
        {
            if (v1 >= 0)
                printf("= %" PRId32 "/%" PRId32 " = %" PRId32 "+%" PRId32 "/%" PRId32 "", (v * v2 + v1), v2, v, v1, v2);
            else
                printf("= %" PRId32 "/%" PRId32 " = %" PRId32 "%" PRId32 "/%" PRId32 "", (v * v2 + v1), v2, v, v1, v2);
        }
        else
            printf("= %" PRId32 "/%" PRId32 "", v1, v2);
        if (wasdecimal == false)
        {
            s_process(exp, nextop(exp, 1));
            printf(" = %s\n", exp);
        }
        // Remember to not leak memory
        free(f_v1);
        free(f_v2);
    }
}

bool decimal_to_fraction(char *exp, bool inverse_process)
{
    int p = 0, i, j, fractionl;
    int32_t v1, v2 = 0;
    char pattern[14];
    double v = 0;
    bool success = false;
    // we read the value and rewrite it in case of scientific notation
    // ex: 1e-7 -> rewrite to 0.0000001
    sscanf(exp, "%lf", &v);
    if (fabs(v) > 2147483647 || v - (int32_t)v == 0 || fabs(v) < 1e-8)
        return false;
    if (fabs(v) < 1e-4)
    {
        sprintf(exp, "%.16lf", v);
        // Removing trailing zeros
        for (i = strlen(exp) - 1; i > p; --i)
        {
            // Stop at the first non zero value and null terminate
            if (*(exp + i) != '0')
            {
                *(exp + i + 1) = '\0';
                break;
            }
        }
    }
    for (i = 0; i < strlen(exp); ++i)
        if (*(exp + i) == '.')
        {
            p = i;
            break;
        }
    // return if no point is found
    if (p == 0)
        return false;
    fractionl = strlen(exp) - p - 1;
    if (fractionl >= 10)
    {
        // Look for a pattern to detect fractions from periodic decimals
        for (i = p + 1; i - p < fractionl / 2; ++i)
        {
            pattern[0] = *(exp + i);
            pattern[1] = '\0';
            j = i + 1;
            while (true)
            {
                // First case: the "pattern" is smaller than the remaining decimal digits.
                if (strlen(exp) - j > strlen(pattern))
                {
                    // If the pattern is found again in the remaining decimal digits, jump over it.
                    if (strncmp(pattern, exp + j, strlen(pattern)) == 0)
                        j += strlen(pattern);
                    // If not, copy the digits between i and j to the pattern for further testing
                    else
                    {
                        strncpy(pattern, exp + i, j - i + 1);
                        pattern[j - i + 1] = '\0';
                        ++j;
                    }
                    // If the pattern matches the last decimal digits, stop the execution with SUCCESS
                    if (strlen(exp) - j == strlen(pattern) && strncmp(pattern, exp + strlen(exp) + j, strlen(pattern) == 0))
                    {
                        success = true;
                        break;
                    }
                }
                // Second case: the pattern is bigger than the remaining digits.
                else
                {
                    // Consider a SUCCESS the case where the remaining digits except the last one match the leftmost digits of the pattern
                    if (strncmp(pattern, exp + j, strlen(exp) - j - 1) == 0)
                    {
                        success = true;
                        break;
                    }
                    else
                        break;
                }
            }
            if (success == true)
                break;
        }
        // getting the fraction from the "pattern"
        if (success == true)
        {
            int i2;
            if (strlen(pattern) + i - p - 1 < 8)
            {
                for (i2 = 0; i2 < strlen(pattern); ++i2)
                    v2 += 9 * pow(10, i2);
                v2 = v2 * pow(10, i - p - 1);
            }
            else
                return false;
            sscanf(pattern, "%" PRId32 " ", &v1);
            // i - p - 1 is the number of decimal digits between the period and the point
            v1 = v1 + (int32_t)(v * pow(10, i - p - 1)) * (v2 / pow(10, i - p - 1));
            sprintf(exp, "%" PRId32 "/%" PRId32 "", v1, v2);
            if (inverse_process == false)
            {
                // The ~ indicates that the fraction does not equal the decimal, but it is very close to it
                printf("~");
                fraction_processor(exp, true);
            }
            return true;
        }
    }
    else if (strlen(exp) - 1 < 10)
    {
        // Prevent integer overflow by returning if the value resulting will have more than 9 digits
        v2 = pow(10, fractionl);
        v1 = v * v2;
        sprintf(exp, "%" PRId32 "/%" PRId32 "", v1, v2);
        if (inverse_process == false)
        {
            fraction_processor(exp, true);
        }
        return true;
    }
    // trying to get the fraction by inverting it then trying the algorithm again
    if (inverse_process == false && fractionl > 10)
    {
        double inverse = 1 / v;
        char temp[22];
        // case where the fraction is likely of form 1/x
        if (fabs(inverse - round(inverse)) < 1e-7)
        {
            if (fabs(round(inverse)) == 1)
                printf("~=1");
            else
                printf("~=1/%" PRId32 "", (int32_t)round(inverse));
            return true;
        }
        sprintf(temp, "%.12lf", inverse);
        if (decimal_to_fraction(temp, true) == false)
            return false;
        else
        {
            sscanf(temp, "%" PRId32 "/%" PRId32 "", &v2, &v1);
            sprintf(exp, "%" PRId32 "/%" PRId32 "", v1, v2);
            printf("~");
            fraction_processor(exp, true);
            return true;
        }
    }
    return false;
}
void priority_fill(node *list, int op_count)
{
    char operators[6] = {'^', '*', '/', '%', '+', '-'};
    uint8_t priority[6] = {3, 2, 2, 2, 1, 1};
    int i, j;
    for (i = 0; i < op_count; ++i)
    {
        for (j = 0; j < 7; ++j)
        {
            if (list[i].operator== operators[j])
            {
                list[i].priority = priority[j];
                break;
            }
        }
    }
}
// Function that finds the subexpression that starts from 'start'
// Mode determines if the value to return is expression_start (1) or solve_start (2)
int subexp_start_at(s_expression *expression, int start, int current_s_exp, int mode)
{
    int i;
    i = current_s_exp - 1;
    switch (mode)
    {
    case 1:
        while (i >= 0)
        {
            if (expression[i].expression_start == start)
                return i;
            else
                --i;
        }
        break;
    case 2:
        while (i >= 0)
        {
            if (expression[i].solve_start == start)
                return i;
            else
                --i;
        }
    }
    return -1;
}
// Function that finds the subexpression that ends at 'end'
int s_exp_ending(s_expression *expression, int end, int current_s_exp, int s_exp_count)
{
    int i;
    i = current_s_exp - 1;
    while (i < s_exp_count)
    {
        if (expression[i].solve_end == end)
            return i;
        else
            --i;
    }
    return -1;
}