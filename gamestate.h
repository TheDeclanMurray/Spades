#pragma once

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define END_SCORE 500

typedef enum {
    SPADE = 0,
    CLUB = 1,
    HEART = 2,
    DIAMOND = 3
} SUITE;

typedef struct client
{
    int client_num;
    int *client_hand;
    int hand_size;
    int bid;
    int points;
} client_t;

typedef struct gamestate
{
    client_t *clients;
    int *bot_clients;
    int num_bots;


    int current_leader;
    int dealer;
    bool spades_broken;
    int trick_suit;
    int trick_high;

    int total_score_team1;
    int total_score_team2;
    int current_player;

    int tricks_over_team1;
    int tricks_over_team2;


} gamestate_t;

int findSuit(int card);

int findRank(int card);

gamestate_t *init_game(int num_bots);

bool isLegalMove(gamestate_t gs, int card);

void swap(int i, int j, int *arr);

void shuffle(int *cards);

char *format_hand(int *hand, int size);

int *create_cards();

void update_dealer_and_lead_player(gamestate_t *state);

void deal_cards(gamestate_t *state, int *cards);

//was main
int run_game(gamestate_t *gamestate);