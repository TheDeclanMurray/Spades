#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <stdint.h>
#include "gamestate.h"
#include "send_messages.h"

/**
 * Returns the suit of a card
 * 0 = Spades
 * 1 = Clubs
 * 2 = Hearts
 * 3 = Diamonds
 */
int findSuit(int card)
{
    return card / 13;
}

/**
 * 1 = Ace
 * 2-10 the same
 * 11 = Jack
 * 12 = Queen
 * 13 = King
 */
int findRank(int card)
{
    return card % 13 + 1;
}

/**
 * Initializes a gamestate when creating the game.
 */
gamestate_t *init_game(int num_bots)
{
    /* Malloc memory needed for the struct and the client structs*/
    gamestate_t *state = malloc(sizeof(gamestate_t));

    client_t *clients = malloc(sizeof(client_t) * 4);

    /* Malloc memory for the client hands*/
    for (int i = 0; i < 4; i++)
    {
        client_t *client = malloc(sizeof(client_t));
        client->client_num = i;
        // client->client_hand = malloc(sizeof(int) * 13);
        // TODO: Initialize client hand
        client->bid = -1;
        client->points = 0;
        client->hand_size = 13;
        clients[i] = *client;
    }

    // TODO: Check incoming connections, assign bots to 4 - |incoming connections|
    int *bot_clients = malloc(sizeof(int) * num_bots);
    for (int i = 0; i < num_bots; i++)
    {
        bot_clients[i] = i;
    } // Note that this means the first num_bots number of clients will be assigned to be bots

    /* Set initial variables to be what makes sense (usually either -1 or 0 depending on the type)*/
    int current_leader = -1;

    int dealer = -1;

    bool spades_broken = false;

    state->clients = clients;
    state->bot_clients = bot_clients;
    state->current_leader = current_leader;
    state->dealer = dealer;
    state->current_player = -1;
    state->spades_broken = spades_broken;
    state->total_score_team1 = 0;
    state->total_score_team2 = 0;
    state->trick_high = -1;
    state->trick_suit = -1;
    state->tricks_over_team1 = 0;
    state->tricks_over_team2 = 0;
    state->highest_spade = -1;
    state->played_cards = malloc(sizeof(int) * 4);
    // state->sockets = malloc(sizeof(intptr_t) *4);
    state->usernames = malloc(sizeof(char *) * 4);

    return state;
}

/**
 * Returns true if card is legal play false otherwise
 */
bool isLegalMove(gamestate_t *gs, int card)
{
    /* Store variables locally for better readability*/
    int activePlayer = gs->current_player;
    int *hand = gs->clients[activePlayer].client_hand;
    int hand_size = gs->clients[activePlayer].hand_size;
    bool hasCard = false;
    bool hasSuit = false;
    int trick_suit = gs->trick_suit;

    // checks hand for card
    for (int i = 0; i < hand_size; i++)
    {
        if (card == hand[i])
        {
            hasCard = true;
        }
    }
    // if player doesn't have card return flase
    if (hasCard == false)
    {
        return false;
    }

    // checks if player is active player
    if (activePlayer == gs->current_leader)
    {
        // checks if spades are broken if the card played is a spade
        if ((findSuit(card) == SPADE) && !gs->spades_broken)
        {
            // checks if player only has spades to make this a legal move
            for (int i = 0; i < hand_size; i++)
            {
                if (findSuit(hand[i]) != SPADE)
                {
                    // returns false if spade is lead while other cards are in hand
                    return false;
                }
            }
        }
        // returns true otherwise
        return true;
    }

    // finds if they have trick_suit in hand
    for (int i = 0; i < hand_size; i++)
    {
        if (findSuit(hand[i]) == trick_suit)
        {
            hasSuit = true;
        }
    }

    // if they have suit but played a card that doesn't match it
    if (hasSuit && (findSuit(card) != trick_suit))
    {
        return false;
    }
    return true;
}

/**
 * Swap two indexes in an array.
 */
void swap(int i, int j, int *arr)
{
    int temp = arr[i];
    arr[i] = arr[j];
    arr[j] = temp;
}

/**
 * Shuffle a deck of cards by swapping them randomly around. Fischer-Yates shuffle.
 */
void shuffle(int *cards)
{
    for (int i = 51; i > 0; i--)
    {
        int j = rand() % (i + 1);
        swap(i, j, cards);
    }
}

/**
 * Obtain the string version of a card with unicode-num form.
 */
char *find_card(int card)
{
    /* Malloc memory for the card, probably more than needed*/
    char *msg = malloc(sizeof(char) * 16);
    strcpy(msg, ""); // wipe the memory so that we don't get weird chars when we concat.
    int suit = card / 13;
    int num = (card % 13) + 1;

    // Citation: https://stackoverflow.com/questions/27133508/how-to-print-spades-hearts-diamonds-etc-in-c-and-linux
    char *suit_chars[4] = {"\xE2\x99\xA0", "\xE2\x99\xA3", "\xE2\x99\xA5", "\xE2\x99\xA6"};
    char *formatted_nums[14] = {"0", "A", "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K"};

    /* Concatenate based on our arrays above.*/
    strcat(msg, suit_chars[suit]);
    strcat(msg, formatted_nums[num]);
    strcat(msg, "\n");
    return msg;
}

/**
 * Format an entire hand for printing.
 * @returns msg, a malloced piece of memory
 */
char *format_hand(int *hand, int size)
{
    int element = 1;

    // Sort the cards using insertion sort
    while (element < size)
    {
        for (int j = element; j > 0; j--)
        {
            if (hand[j] < hand[j - 1])
            {
                swap(j, j - 1, hand);
            }
            else
            {
                element++;
                break;
            }
        }
    }

    // Format the cards into a character array, wipe memory clean so we don't get weird chars
    char *msg = (char *)malloc(256 * sizeof(char));
    strcpy(msg, "");

    /* Create an array of dashes, to act as the top and bottoms of cards*/
    char *dashes = (char *)malloc(sizeof(char) * size * 3 + 1);
    strcpy(dashes, "");

    for (int i = 0; i < size * 3 + 1; i++)
    {
        strcat(dashes, "-");
    }
    strcat(dashes, "\n");

    strcat(msg, dashes);
    strcat(msg, "|");

    /*Add the actual cards (similar logic to above) to the formatted string*/
    for (int i = 0; i < size; i++)
    {
        int card = hand[i];
        int suit = card / 13;
        int num = (card % 13) + 1;

        // Citation: https://stackoverflow.com/questions/27133508/how-to-print-spades-hearts-diamonds-etc-in-c-and-linux
        char *suit_chars[4] = {"\xE2\x99\xA0", "\xE2\x99\xA3", "\xE2\x99\xA5", "\xE2\x99\xA6"};

        char *formatted_nums[14] = {"0", "A", "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K"};

        // find_card(hand[i]);

        strcat(msg, suit_chars[suit]);
        strcat(msg, formatted_nums[num]);
        strcat(msg, "|");
    }
    strcat(msg, "\n");
    strcat(msg, dashes);

    /* Free the memory we can*/
    free(dashes);

    return msg;
}

/* Create the deck of cards.*/
int *create_cards()
{
    int *cards = malloc(sizeof(int) * 52);
    for (int i = 0; i < 52; i++)
    {
        cards[i] = i;
    }
    return cards;
}

/* Update the dealer at the beginning of each round randomly, set the first player to be the player to the left.*/
void update_dealer_and_lead_player(gamestate_t *state)
{
    /* Time seed for better randomness*/
    srand(time(NULL));

    /* Set the dealer randomly the first time, rotate after that.*/
    if (state->dealer == -1)
    {
        state->dealer = rand() % 4;
    }
    else
    {
        state->dealer++;
        if (state->dealer >= 4)
        {
            state->dealer = 0;
        }
    }

    /* Current player is the player to the left of the dealer.*/
    state->current_player = state->dealer + 1;
    if (state->current_player >= 4)
    {
        state->current_player = 0;
    }
    state->current_leader = state->current_player;
}

/**
 * Deal the cards to each player (partition the deck into 4 pieces)
 */
void deal_cards(gamestate_t *state, int *cards)
{
    /* For each clinet, create their hand (and remalloc the memory that we shrunk).*/
    for (int i = 0; i < 4; i++)
    {
        state->clients[i].hand_size = 13;
        state->clients[i].client_hand = malloc(sizeof(int) * 13);
    }

    /* Partition the cards.*/
    for (int i = 0; i < 52; i++)
    {
        if (i < 13)
        {
            state->clients[0].client_hand[i] = cards[i];
        }
        else if (i >= 13 && i < 26)
        {
            state->clients[1].client_hand[(i - 13)] = cards[i];
        }
        else if (i >= 26 && i < 39)
        {
            state->clients[2].client_hand[(i - 26)] = cards[i];
        }
        else if (i >= 39)
        {
            state->clients[3].client_hand[(i - 39)] = cards[i];
        }
    }
}

/**
 * Apply a legal move and update the gamestate.
 */
void playMove(gamestate_t *state, int card_played)
{
    /* Update the suit if first time played*/
    if (state->trick_suit == -1)
    {
        state->trick_suit = findSuit(card_played);
    }

    /* If the player would be winning with a nonspade (or a first spade)*/
    if (((findRank(card_played) > state->trick_high) || (findRank(card_played) == 1)) && (findSuit(card_played) == state->trick_suit) && (state->highest_spade == -1))
    {
        // player is now winning the trick
        state->current_leader = state->current_player;
        if (findSuit(card_played) == 0)
        {
            /* If they led with a spade update all that stuff*/
            state->highest_spade = findRank(card_played);
            state->spades_broken = true;
            if (findRank(card_played) == 1) {
                state->highest_spade = 1000;
            }
        }
        else
        {
            /* It's not a spade. Aces are a lot of points because they're the best.*/
            state->trick_high = findRank(card_played);
            if (state->trick_high == 1)
            {
                state->trick_high = 1000; // For aces
            }
        }
    }
    /* It's a spade and spades have already been played*/
    else if (findSuit(card_played) == 0)
    {
        state->spades_broken = true;
        /* It's a spade*/
        if ((findRank(card_played) > state->highest_spade) || (findRank(card_played) == 1))
        {
            state->highest_spade = findRank(card_played);
            if (state->highest_spade == 1)
            {
                state->highest_spade = 1000;
            }
            // player is now winning the trick
            state->current_leader = state->current_player;
        }
    }
    /* Get a pointer the client for easy access*/
    client_t *client = &state->clients[state->current_player];

    /* Remove the card from the clients hand and realloc the memory to be smaller.*/
    for (int i = 0; i < client->hand_size; i++)
    {
        if (client->client_hand[i] == card_played)
        {
            client->client_hand[i] = -1;
            swap(i, client->hand_size - 1, client->client_hand);
            client->hand_size = client->hand_size - 1;
            client->client_hand = realloc(client->client_hand, client->hand_size * sizeof(int));
        }
    }

    /* Rotate the current player */
    state->played_cards[state->current_player] = card_played;
    if (state->current_player == 3)
    {
        state->current_player = 0;
    }
    else
    {
        state->current_player++;
    }
}

void update_points(gamestate_t *state)
{
    /* Total points and bids for team 1*/
    int total_points1 = state->clients[0].points + state->clients[2].points;
    int total_bids1 = state->clients[0].bid + state->clients[2].bid;

    /* If team 1 made their bid*/
    if (total_bids1 <= total_points1)
    {
        state->tricks_over_team1 += total_points1 - total_bids1;
        state->total_score_team1 += total_bids1 * 10 + (total_points1 - total_bids1);
    }

    /* If team 1 did not make their bid*/
    if (total_bids1 > total_points1)
    {
        state->total_score_team1 -= total_bids1 * 10;
    }

    /* If team one went over by too much*/
    if (state->tricks_over_team1 >= 10)
    {
        state->total_score_team1 -= 100;
        state->tricks_over_team1 -= 10;
    }

    /* Repeat identical logic for team 2*/
    int total_points2 = state->clients[1].points + state->clients[3].points;
    int total_bids2 = state->clients[1].bid + state->clients[3].bid;
    if (total_bids2 <= total_points2)
    {
        state->tricks_over_team2 += total_points2 - total_bids2;
        state->total_score_team2 += total_bids2 * 10 + (total_points2 - total_bids2);
    }
    if (total_bids2 > total_points2)
    {
        state->total_score_team2 -= total_bids2 * 10;
    }
    if (state->tricks_over_team2 >= 10)
    {
        state->total_score_team2 -= 100;
        state->tricks_over_team2 -= 10;
    }
}

/**
 * Main game logic function that actually runs the thread
 */
int run_game(gamestate_t *gamestate, pthread_mutex_t *lock, pthread_cond_t *cond, char *action)
{
    int *cards = create_cards();

    /* Run the game loop as long as someone hasn't won.*/
    while ((gamestate->total_score_team1 < END_SCORE) && (gamestate->total_score_team2 < END_SCORE))
    {
        update_dealer_and_lead_player(gamestate);
        // Randomly choose the first dealer, set current player to be player to the left
        shuffle(cards);
        deal_cards(gamestate, cards);
        for (int player = 0; player < 4; player++)
        {
            char *player_hand = format_hand(gamestate->clients[player].client_hand, gamestate->clients[player].hand_size);

            /* Send the hand to the player and free the mem*/
            send_dm(player_hand, gamestate->sockets[player]);
            free(player_hand);
        }

        /* Send the initial message broadcasting the teams.*/
        char init_message[256];
        sprintf(init_message, "Teams are:\n %s and %s\n %s and %s\n Player %s leads.\n", gamestate->usernames[0], gamestate->usernames[2], gamestate->usernames[1], gamestate->usernames[3], gamestate->usernames[gamestate->current_player]);
        broadcast(init_message, gamestate->sockets);

        /* For each player during a bidding round.*/
        for (int i = 0; i < 4; i++)
        {

            /*Request for a bid*/
            send_dm("Enter your bid: \n", gamestate->sockets[gamestate->current_player]);
            pthread_mutex_lock(lock);
            pthread_cond_wait(cond, lock);
            pthread_mutex_unlock(lock);

            /* Receive the bid and print it.*/
            int bid = atoi(action);
            gamestate->clients[gamestate->current_player].bid = bid;

            char bid_message[256];
            sprintf(bid_message, "Player %s bid: %d\n", gamestate->usernames[gamestate->current_player], bid);

            /* Broadcast the bid to everyone else*/
            broadcast(bid_message, gamestate->sockets);
            gamestate->current_player++;
            if (gamestate->current_player == 4)
            {
                gamestate->current_player = 0;
            }
        }

        /* For each hand, do the following:*/
        int cards_played = 1;
        while (cards_played++ <= 13)
        {
            /* For each round*/
            for (int j = 0; j < 4; j++)
            {

                if (gamestate->num_bots < j + 1)
                {
                    send_dm("Enter your card <SUIT><RANK>:\n", gamestate->sockets[gamestate->current_player]);

                    /* If we are a real player*/
                    pthread_mutex_lock(lock);
                    pthread_cond_wait(cond, lock);
                    pthread_mutex_unlock(lock);

                    /* Obtain player input and translate to int.*/
                    char suit = toupper(action[0]);
                    char *number = action + 1;
                    int suit_num;

                    switch (suit)
                    {
                    case ('S'):
                        suit_num = 0;
                        break;
                    case ('C'):
                        suit_num = 1;
                        break;
                    case ('H'):
                        suit_num = 2;
                        break;
                    default:
                        suit_num = 3;
                        break;
                    } 

                    int card_played = atoi(number) + suit_num * 13 - 1;
                    
                    /* Check if the move is legal.*/
                    bool is_legal = isLegalMove(gamestate, card_played);
                    int real_current_player = gamestate->current_player;

                    /* If it is legal we can actually play the move and update the game*/
                    if (is_legal)
                    {
                        playMove(gamestate, card_played);

                        /* Broadcast the players move to everyone*/
                        char *played_card = find_card(card_played);
                        char card_played_message[26];
                        sprintf(card_played_message, "%.10s played: ", gamestate->usernames[real_current_player]);
                        strcat(card_played_message, played_card);

                        broadcast(card_played_message, gamestate->sockets);
                        free(played_card);

                        /* Send the updated hand back to the player*/
                        char *player_hand = format_hand(gamestate->clients[real_current_player].client_hand, gamestate->clients[real_current_player].hand_size);
                        send_dm(player_hand, gamestate->sockets[real_current_player]);

                        free(player_hand);
                    }
                    else
                    {
                        /* Tell the player they made an illegal move and repeat the step for the same player*/
                        char *bad_message = "Illegal move\n";
                        send_dm(bad_message, gamestate->sockets[gamestate->current_player]);
                        j--;
                    }
                }
                else
                {
                    /* If we are a bot, attempt to generate a random move to play.*/
                    // Currently never run, but should work
                    int *legal_moves = malloc(sizeof(int) * gamestate->clients[gamestate->current_player].hand_size);
                    int num_legal_moves = 0;
                    int move;
                    for (int i = 0; i < gamestate->clients[gamestate->current_player].hand_size; i++)
                    {
                        if (isLegalMove(gamestate, gamestate->clients[gamestate->current_player].client_hand[i]))
                        {
                            legal_moves[num_legal_moves++] = gamestate->clients[gamestate->current_player].client_hand[i];
                        }
                    }
                    move = legal_moves[rand() % num_legal_moves];
                    playMove(gamestate, move);

                    free(legal_moves);

                } // Bot
            }
            /* Update after each hand*/
            // Increment points for winning client
            gamestate->clients[gamestate->current_leader].points++;

            gamestate->trick_suit = -1;
            gamestate->trick_high = -1;
            gamestate->highest_spade = -1;

            /* reset the played cards to be 0*/
            for (int i = 0; i < 4; i++)
            {
                gamestate->played_cards[i] = 0;
            }

            gamestate->current_player = gamestate->current_leader;

            /* Broadcast the winner of the hand and updated points totals.*/
            char winner[256];
            sprintf(winner, "%s won that hand!\n %s points: %d, %s bid: %d\n %s points: %d, %s bid: %d\n %s points: %d, %s bid: %d\n %s points: %d, %s bid: %d\n",
                    gamestate->usernames[gamestate->current_leader],
                    gamestate->usernames[0], gamestate->clients[0].points, gamestate->usernames[0], gamestate->clients[0].bid,
                    gamestate->usernames[1], gamestate->clients[1].points, gamestate->usernames[1], gamestate->clients[1].bid,
                    gamestate->usernames[2], gamestate->clients[2].points, gamestate->usernames[2], gamestate->clients[2].bid,
                    gamestate->usernames[3], gamestate->clients[3].points, gamestate->usernames[3], gamestate->clients[3].bid);
            broadcast(winner, gamestate->sockets);
        }
        /*Update after a round*/
        gamestate->spades_broken = false;
        update_points(gamestate);

        /* Send the game point totals to all players*/
        char points_totals[256];
        sprintf(points_totals, "\nCurrent point totals: \n %s and %s: %d (Tricks over: %d) \n %s and %s: %d (Tricks over: %d)\n",
                gamestate->usernames[0], gamestate->usernames[2], gamestate->total_score_team1, gamestate->tricks_over_team1,
                gamestate->usernames[1], gamestate->usernames[3], gamestate->total_score_team2, gamestate->tricks_over_team2);

        broadcast(points_totals, gamestate->sockets);
        for (int i = 0; i < 4; i++)
        {
            gamestate->clients[i].bid = -1;
            gamestate->clients[i].points = 0;
        }
    }

    /* After the end of the game, tell the players which team won.*/
    if (gamestate->total_score_team1 > gamestate->total_score_team2)
    {
        // Broadcast gamewinner team 1 to clients
        broadcast("Team 1 wins!", gamestate->sockets);
    }
    else
    {
        // Broadcast other winner to clients.
        broadcast("Team 2 wins!", gamestate->sockets);
    }

    return 0;
}
