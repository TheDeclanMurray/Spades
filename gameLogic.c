#include <stdio.h>
#include <stdbool.h>

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
    //this is how we are keeping track of who is starting the trick
    int current_leader;
    int dealer;
    bool spades_broken;
    int total_score_team1;
    int total_score_team2;
    int current_player;
    int trick_suit;
    int trick_high;
    int tricks_over_team1;
    int tricks_over_team2;

} gamestate_t;

/**
 * Returns the suit of a card
 * 0 = Spades
 * 1 = Clubs
 * 2 = Hearts
 * 3 = Diamonds
 */
int findSuit(int card) {
    return card / 13;
}

/**
 * 1 = Ace
 * 2-10 the same
 * 11 = Jack
 * 12 = Queen
 * 13 = King
 */
int findRank(int card) {
    return card % 13 + 1;
}


/**
 * Returns true if card is legal play false otherwise
 */
bool isLegalMove(gamestate_t gs, int card){
    // TODO: server blocks wrong player from playing card
    int activePlayer = gs.current_player;
    int* hand = gs.clients[activePlayer].client_hand;
    int hand_size = gs.clients[activePlayer].hand_size;
    bool hasCard = false;
    bool hasSuit = false;
    int trick_suit = gs.trick_suit;

    //checks hand for card
    for(int i = 0; i < hand_size; i++) {
        if (card == hand[i]){
            hasCard = true;
        }
    }
    //if player doesn't have card return flase
    if(hasCard == false) {
        return false;
    }

    //checks if player is active player
    if(activePlayer == gs.current_leader) {
        //checks if spades are broken if the card played is a spade
        if((findSuit(card) == SPADE) && !gs.spades_broken) {
            //checks if player only has spades to make this a legal move
            for(int i = 0; i < hand_size; i++) {
                if(findSuit(hand[i]) != SPADE) {
                    //returns false if spade is lead while other cards are in hand
                    return false;
                }
            }
        }
        //returns true otherwise
        return true;
    }

    //finds suit of card
    for (int i = 0; i < hand_size; i++){
        if(findSuit(hand[i]) == trick_suit) {
            hasSuit = true;
        }
    }

    //if they have suit but played a card that doesn't match it
    if(hasSuit && (findSuit(card) != trick_suit)) {
        return false;
    }
    return true;
}


gamestate_t *init_game()
{
    gamestate_t *state = malloc(sizeof(gamestate_t));

    client_t *clients = malloc(sizeof(client_t) * 4);

    for (int i = 0; i < 4; i++)
    {
        client_t *client = malloc(sizeof(client_t));
        client->client_num = i;
        client->client_hand = calloc(sizeof(int) * 13);
        // TODO: Initialize client hand
        client->bid = -1;
        client->points = 0;
        client->hand_size = 13;
        clients[i] = *client;
    }

    // TODO: Check incoming connections, assign bots to 4 - |incoming connections|
    int *bot_clients = malloc(sizeof(int) * 4);

    int current_leader = -1;

    int dealer = -1;

    bool spades_broken = false;

    state->clients = clients;
    state->bot_clients = bot_clients;
    state->current_leader = current_leader;
    state->dealer = dealer;
    state->current_player = -1;
    state->spades_broken = spades_broken;
    state->trick_high = -1;
    state->trick_suit = -1;
    state->total_score_team1 = 0;
    state->total_score_team2 = 0;
    state->tricks_over_team1 = 0;
    state->tricks_over_team2 = 0;

    return state;
}

void shuffle(int *cards)
{
    for (int i = 0; i < 52; i++)
    {
        int j = rand() % (52 - (i + 1));
        swap(i, j, cards);
    }
}

void swap(int i, int j, int *arr)
{
    int temp = arr[i];
    arr[i] = arr[j];
    arr[j] = temp;
}

int main()
{
    gamestate_t *gamestate = init_game();
    int *cards = malloc(sizeof(int) * 52);
    for (int i = 0; i < 52; i++)
    {
        cards[i] = i;
    }
    srand(time(NULL));
    while ((gamestate->total_score_team1 < END_SCORE) && (gamestate->total_score_team2 < END_SCORE))
    {

        /* For each hand, do the following:*/
        int cards_played = 1;
        while (cards_played++ <= 13)
        {
            // Randomly choose the first dealer, set current player to be player to the left
            if (gamestate->dealer == -1)
            {
                gamestate->dealer = rand() % 4;
            }
            else
            {
                gamestate->dealer++;
                if (gamestate->dealer >= 4)
                {
                    gamestate->dealer = 0;
                }
            }
            gamestate->current_player = gamestate->dealer + 1;
            if (gamestate->current_player >= 4)
            {
                gamestate->current_player = 0;
            }

            shuffle(cards);

            for (int i = 0; i < 52; i++)
            {
                if (i < 13)
                {
                    gamestate->clients[0].client_hand[i] = cards[i];
                }
                else if (i >= 13 && i < 26)
                {
                    gamestate->clients[1].client_hand[(i - 13)] = cards[i];
                }
                else if (i >= 26 && i < 39)
                {
                    gamestate->clients[2].client_hand[(i - 26)] = cards[i];
                }
                else if (i >= 39)
                {
                    gamestate->clients[3].client_hand[(i - 39)] = cards[i];
                }
            }
        }
    }

    // Send players their hands
    for (int i = 0; i < 4; i++)
    {
        int element = 1;
        while (element < 13)
        {
            for (int j = element; j > 0; j--)
            {
                if (gamestate->clients[i].client_hand[j] > gamestate->clients[i].client_hand[j - 1])
                {
                    swap(j, j - 1, gamestate->clients[i].client_hand);
                }
                else
                {
                    element++;
                    break;
                }
            }
        }
    }

    // for (int i = 0; i < 4; i++) {
    //     char* msg = "";
    //     for (int j = 0; j < 13; j++) {
    //         int card = gamestate->clients[i].client_hand[j];
    //         // Order: spades, clubs, hearts, diamonds
    //         int suit = card / 13;
    //         switch(suit) {
    //             case 0: 
    //                 strcat(msg, '')
    //         }
    //     }
    // }


    // Receive bids in clockwise order

    // Repeat x4:
    // Check if bot_client, do other stuff if is

    // Receive current players move

    // Check that move is legal

    // Update gamestate based on move

    // Broadcast the move to clients

    // Update points based on bid/ tricks won, check tricks over
    // Broadcast points total


    if (gamestate->total_score_team1 > gamestate->total_score_team2)
    {
        // Broadcast gamewinner team 1 to clients
    }
    else
    {
        // Broadcast other winner to clients.
    }

return 0;
}
