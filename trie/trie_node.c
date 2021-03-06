#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "trie.h"
#include "../tools.h"
#include "trie_node.h"
// #include "hash_table.h"

void init_trie_node(trie_node * node)
{
    if(node == NULL)
        error_exit("Malloc Failure");

    node->is_final = NO;
    node->current_children = 0;
    node->a_version=0;
    node->d_version= NOT_DELETED;
    node->max_children = NUMBER_OF_CHILDREN;
    node->word=NULL;
    node->compressed = NULL;
    node->children = NULL;
    // node->children = malloc(node->max_children * sizeof(trie_node));
}

OK_SUCCESS insert_ngram_to_node(trie_node * node, char * ngram, int version)
{
    trie_node * temp = node;

    char * word = strtok(NULL, " ");
    if( word == NULL )
    {
        node->a_version = version;
        node->is_final = YES;
        return 1;
    }
    int a , b , m, found_word;
    int spot=0;

    while(1)
    {
        if(temp->current_children == 0 )
        { /*we need to add a new child cause this node has 0 of them */
            if(temp->children==NULL){
                temp->children=malloc(temp->max_children*sizeof(trie_node));
            }
            init_trie_node(&temp->children[0]);
            temp->children[0].word = copy_string(word);
            // temp->children[0].a_version = version;
            spot = 0;
            temp->current_children ++;
            // printf("We added the word %s because the word before has no children\n", word);
        }
        else
        {
            found_word = binary_search_kid(temp, word, &spot);
            if( found_word==-1)
            {/*add a new child at the {spot} position*/
                if(temp->current_children == temp->max_children)
                {/*we need to give more space to our children's array*/
                    temp->max_children *=2;
                    temp->children = realloc(temp->children, temp->max_children*sizeof(trie_node));
                }
                int i;
                /*move the elements so we can make space for the new child*/
                memmove(&temp->children[spot+1], &temp->children[spot], (temp->current_children-spot)*sizeof(trie_node));
                /*create the new child*/
                init_trie_node(&temp->children[spot]);
                temp->children[spot].word = copy_string(word);
                // temp->children[spot].a_version = version;

                temp->current_children ++;

                // printf("added the word %s \n", word);
            }
            else if(temp->children[spot].d_version > temp->children[spot].a_version )
            {
                // temp->children[spot].a_version = version;
            }
        }

        word = strtok(NULL, " ");
        if (word == NULL)
        {/*end of N-Gram*/
            temp->children[spot].a_version = version;
            temp->children[spot].is_final = YES;
            return 1;
        }
        /*next word of our N-Gram*/
        temp = &temp->children[spot];
    }
}

OK_SUCCESS delete_node_child(trie_node* node,int position)
{   /* free the child */
    free(node->children[position].children);
    free(node->children[position].word);

    /*fill the gap in the children array that created due to the removal*/
    /*delete the last child because it is dublicate*/
    if(node->current_children-1!=position && (node->current_children-1)>0){
        memmove(&node->children[position], &node->children[position+1], (node->current_children-position-1)*sizeof(trie_node));
    }
    node->current_children--;

    return 1;
}

OK_SUCCESS trie_delete(hash_table* table,char* ngram,int current_version)
{
    int position,found,stack_count=0;
    int stack_size=count_words(ngram);
    //create a stack to hold the node that we need and their position in the table of their upper node
    stack_node* stack;
    stack=malloc(stack_size*sizeof(stack_node));

    trie_node* temp_trie_node;
    trie_node* hash_node;
    //if the word is null return
    if(ngram == NULL || strcmp(ngram, "")==0)
    {
        free(stack);
        return -1;
    }
    char* hash_word;
    char* word=strtok(ngram," ");
    if(word==NULL){
        free(stack);
        return -1;
    }

    hash_word=malloc((strlen(word)+1)*sizeof(char));
    strcpy(hash_word, word);
    hash_node=hash_search(table, word);
    if(hash_node==NULL){
        free(hash_word);
        free(stack);
        return-1;
    }
    word=strtok(NULL, " ");
    temp_trie_node=hash_node;
    //traverse the trie
    while(1)
    {
        if(word==NULL)
        {/*end of N-gram*/
            break;
        }
        found=binary_search_kid(temp_trie_node,word,&position);
        if(found==-1)
        {/*the requested N-Grams doesn't exist */
            free(hash_word);
            free(stack);
            return -1 ;
        }
        if(stack_count>=stack_size)
        {
            free(hash_word);
            free(stack);
            return -1;
        }
        stack[stack_count].node=temp_trie_node;
        stack[stack_count].position=position;
        stack_count++;
        temp_trie_node=&temp_trie_node->children[position];
        /*get next word of our N-Gram*/
        word=strtok(NULL, " ");
    }
    // printf("OK TO STACK\n" );
    stack_count--;
    if(stack_count>=0){
        if(stack[stack_count].node->children[stack[stack_count].position].is_final==YES)
        {/*It must be a final word*/
            stack[stack_count].node->children[stack[stack_count].position].d_version=current_version;
            free(hash_word);
            free(stack);
            return 1;

        }
        else
        {
            free(hash_word);
            free(stack);
            return -1;
        }
        stack_count--;
    }
    else
    {
        if(hash_node->is_final==YES)
        {
            hash_node->d_version=current_version;
            if(hash_node->current_children==0)
            {
                int return_value= hash_delete(table, hash_word,current_version);
                free(hash_word);
                free(stack);
                return return_value;
            }
        }
        free(hash_word);
        free(stack);
        return -1;
    }
    free(hash_word);
    free(stack);
    return 1;

}

int binary_search_array(trie_node* array, int size, char* word, int* spot_ptr_arg){
    int spot = 0;//the current spot
    int a = 0;//the base index in array
    int b = size;//the max index in array
    int found_word = 0;//bool, 1 if found
    int m;//the middle of hte [a,b]
    if(size==0){
        //if no childrens
        if(spot_ptr_arg!=NULL){
            *spot_ptr_arg=0;
        }
        return -1;
    }
    while((a <= b) && (a< size))
    {
        m = (a + b) /2;
        if ( strcmp(word, array[m].word) == 0)
        { /*this sentence exist as far as here*/
            spot = m;
            found_word =1;
            break;
        }
        else if( strcmp(word, array[m].word) < 0)
        {/*word belongs before this child*/
            b = m-1;
            spot = m;
        }
        else if( strcmp(word, array[m].word) > 0)
        {/*word belongs after this child*/
            a = m+1;
            spot = a;
        }
        // printf("A is %d , B is %d\n",a,b );

    }
    if(spot_ptr_arg!=NULL){
        *spot_ptr_arg=spot;
    }
    if(found_word==0){
        return -1;
    }else{
        return 1;
    }
}

int my_cmp(char* word1, char* word2,int max_bytes_word2){
    int res = strncmp(word1, word2,max_bytes_word2);
    if(res==0){
        if(word1[max_bytes_word2]=='\0'){
            return 0;
        }else{
            return 1;
        }
    }else{
        return res;
    }
}

int static_binary_search_array(trie_node* array, int size, char* word, int* spot_ptr_arg){
    int spot = 0;//the current spot
    int a = 0;//the base index in array
    int b = size;//the max index in array
    int found_word = 0;//bool, 1 if found
    int m;//the middle of hte [a,b]
    if(size==0){
        //if no childrens
        if(spot_ptr_arg!=NULL){
            *spot_ptr_arg=0;
        }
        return -1;
    }
    while((a <= b) && (a< size))
    {
        m = (a + b) /2;
        int result_of_cmp  = -1;
        if(array[m].compressed!=NULL){
            result_of_cmp = my_cmp(word, array[m].word, abs(array[m].compressed->positions[0]) );
        }else{
            result_of_cmp = strcmp(word, array[m].word);
        }
        if ( result_of_cmp == 0) { /*this sentence exist as far as here*/
            spot = m;
            found_word =1;
            break;
        }
        else if( result_of_cmp < 0)
        {/*word belongs before this child*/
            b = m-1;
            spot = m;
        }
        else if( result_of_cmp > 0)
        {/*word belongs after this child*/
            a = m+1;
            spot = a;
        }
        // printf("A is %d , B is %d\n",a,b );

    }
    if(spot_ptr_arg!=NULL){
        *spot_ptr_arg=spot;
    }
    if(found_word==0){
        return -1;
    }else{
        return 1;
    }
}

//binary search that returns the index in the array of the children
int binary_search_kid(trie_node* master_node,char* word,int* spot_ptr_arg)
{
    return binary_search_array(master_node->children, master_node->current_children, word, spot_ptr_arg);
}

OK_SUCCESS trie_node_clean(trie_node* node)
{
    int i;
    for(i=0;i<node->current_children;i++)
    {//call the clean for all the children
        trie_node_clean(&node->children[i]);
    }
    if(node->compressed != NULL)
    {
        free(node->compressed->positions);
        free(node->compressed);
    }
    free(node->word);
    if(node->children!=NULL){
        free(node->children);
    }
    return 1;
}

int compress(trie_node *node)
{
    trie_node* count_nodes;
    trie_node* change_nodes;
    trie_node* delete_nodes;
    count_nodes=node;
    int i,final_word_size=1,loops=0;
    //check how many compression must be done and the size of the final word
    while(count_nodes->current_children==1)
    {
        final_word_size+=strlen(count_nodes->children[0].word);
        loops++;
        count_nodes=&count_nodes->children[0];
    }
    if(loops==0)
    {
        return 0;//nothing to change
    }
    final_word_size+=strlen(node->word);//add my word size
    //create the ultra_node
    node->compressed=malloc(sizeof(ultra_node));
    node->compressed->counter=loops+1;
    node->compressed->positions=malloc(sizeof(short)*(loops+1));
    //realloc the compressed word
    node->word=realloc(node->word, final_word_size*sizeof(char));
    //my word in the first position
    node->compressed->positions[0]=strlen(node->word)*node->is_final;
    //take the first children
    change_nodes=node->children;
    for(i = 0 ; i <loops ; i++){
        // printf("eixa %s tha valw %s\n megethos %d \n\n",node->word, change_nodes->word, final_word_size );
        strcat(node->word, change_nodes->word);//add the next word
        node->compressed->positions[i+1]=strlen(change_nodes[0].word)*change_nodes[0].is_final;//write the lenght to the table position
        free(change_nodes[0].word);//free the word
        delete_nodes=change_nodes;//take the node must be deleted
        node->current_children = change_nodes[0].current_children;
        node->max_children = change_nodes[0].max_children;

        change_nodes=change_nodes[0].children;//first take his children
        free(delete_nodes);//then free it
    }
    node->children=change_nodes;//change the node children to the final compressed node's childern
    return 1;
}

void DFS_for_compress(trie_node * root)
{
    trie_node * temp = root;

    trie_node ** stack = malloc(sizeof(trie_node *) * STACK_SIZE);
    int end = 0;
    int max = STACK_SIZE;
    compress(temp);
    int i;
    for(i = 0; i < temp->current_children; i++)
    {   /*add the kids to stack */
        if(end == max)
        {   /* change the size of the stack */
            max *=2;
            stack = realloc(stack, sizeof(trie_node *) * max);
        }
        stack[end] = & temp->children[i];
        end ++;
    }

    while(end > 0)
    {
        /* pop from stack */
        temp = stack[end-1];
        end --;


        compress(temp);
        for(i = 0; i < temp->current_children; i++)
        {   /* add the kids to stack */
            if(end == max)
            {   /* change the size of the stack */
                max *=2;
                stack = realloc(stack, sizeof(trie_node *) * max);
            }
            /* push to stack */
            stack[end] = & temp->children[i];
            end ++;
        }
    }
    free(stack);
}
