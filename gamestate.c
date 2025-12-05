#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define END_SCORE 500

typedef struct client
{
    int client_num;
    int *client_hand;
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
    int total_score_team1;
    int total_score_team2;
    int current_player;

    int tricks_over_team1;
    int tricks_over_team2;

} gamestate_t;

gamestate_t *init_game(int num_bots)
{
    gamestate_t *state = malloc(sizeof(gamestate_t));

    client_t *clients = malloc(sizeof(client_t) * 4);

    for (int i = 0; i < 4; i++)
    {
        client_t *client = malloc(sizeof(client_t));
        client->client_num = i;
        client->client_hand = malloc(sizeof(int) * 13);
        // TODO: Initialize client hand
        client->bid = -1;
        client->points = 0;
        clients[i] = *client;
    }

    // TODO: Check incoming connections, assign bots to 4 - |incoming connections|
    int *bot_clients = malloc(sizeof(int) * num_bots);
    for (int i = 0; i < num_bots; i++) {
        bot_clients[i] = i;
    } // Note that this means the first num_bots number of clients will be assigned to be bots

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
    state->tricks_over_team1 = 0;
    state->tricks_over_team2 = 0;

    return state;
}

void swap(int i, int j, int *arr)
{
    int temp = arr[i];
    arr[i] = arr[j];
    arr[j] = temp;
}

void shuffle(int *cards)
{
    for (int i = 0; i < 52; i++)
    {
        int j = rand() % (52 - (i + 1));
        swap(i, j, cards);
    }
}

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

    // Format the cards into a character array
    char *msg = malloc(sizeof(char) * 256);

    char *dashes = malloc(sizeof(char) * size * 3 + 1);

    for (int i = 0; i < size * 3 + 1; i++)
    {
        strcat(dashes, "-");
    }
    strcat(dashes, "\n");

    strcat(msg, dashes);
    strcat(msg, "|");

    for (int i = 0; i < size; i++)
    {
        int card = hand[i];
        int suit = card / 13;
        int num = (card % 13) + 1;

        // Citation: https://stackoverflow.com/questions/27133508/how-to-print-spades-hearts-diamonds-etc-in-c-and-linux
        char *suit_chars[4] = {"\xE2\x99\xA0", "\xE2\x99\xA3", "\xE2\x99\xA5", "\xE2\x99\xA6"};

        char *formatted_nums[14] = {"0", "A", "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K"};

        strcat(msg, suit_chars[suit]);
        strcat(msg, formatted_nums[num]);
        strcat(msg, "|");
    }
    strcat(msg, "\n");
    strcat(msg, dashes);

    return msg;
}

int *create_cards()
{
    int *cards = malloc(sizeof(int) * 52);
    for (int i = 0; i < 52; i++)
    {
        cards[i] = i;
    }
}

void update_dealer_and_lead_player(gamestate_t *state)
{
    srand(time(NULL));

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
    state->current_player = state->dealer + 1;
    if (state->current_player >= 4)
    {
        state->current_player = 0;
    }
}

void deal_cards(gamestate_t *state, int *cards)
{
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

int main()
{
    // int hand[13] = {0, 3, 4, 8, 25, 26, 41, 20, 16, 44, 51, 33};
    // char *formatted_hand = format_hand(hand, 12);
    // printf("Formatted hand:\n%s\n", formatted_hand);

    // TODO: Receive number of bots in game
    // int bot_num =
    gamestate_t *gamestate = init_game(bot_num);
    int *cards = create_cards();
    while ((gamestate->total_score_team1 < END_SCORE) && (gamestate->total_score_team2 < END_SCORE))
    {
        update_dealer_and_lead_player(gamestate);
        // Randomly choose the first dealer, set current player to be player to the left

        shuffle(cards);

        deal_cards(gamestate, cards);

        for (int player = 0; player < 4; player++)
        {
            char *player_hand = format_hand(gamestate->clients[player].client_hand, 13); // TODO: update hand size with new field

            // TODO: send hand
        }

        for (int player = 0; player < 4; player++)
        {
            // TODO: receive player bid
            int bid = -1; //Update
            gamestate->clients[player].bid = bid;
        }

        /* For each hand, do the following:*/
        int cards_played = 1;
        while (cards_played++ <= 13)
        {
            for (int player_num = 0; player_num < 4; player_num++) {
                if (gamestate->num_bots < player_num + 1) {
                    /* If we are a real player*/

                    //TODO: Send message asking for card to play
                    // TODO: Receive card played
                    // int card_played =
                    // TODO: check if valid card
                    bool is_legal = true; //Update
                    if (is_legal) {
                        // TODO: update game_state based on move
                        // TODO: broadcast the message
                    } else {
                        //TODO: send message saying move is illegal, request again
                    }

                } else {

                } // Bot
            }
        }
    }


    // if (gamestate->total_score_team1 > gamestate->total_score_team2)
    // {
    //     // Broadcast gamewinner team 1 to clients
    // }
    // else
    // {
    //     // Broadcast other winner to clients.
    // }

    return 0;
}
