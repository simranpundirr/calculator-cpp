#include <bits/stdc++.h>
using namespace std;

#define MAX_EV 60
#define MAX_PORTS 20
#define POP_SIZE 120
#define MAX_GEN 400

int N, P;
int arrival[MAX_EV];
int duration_min[MAX_EV];
int urgency_flag[MAX_EV];
int ev_id_map[MAX_EV];

int population[POP_SIZE][MAX_EV];
double fitness_arr[POP_SIZE];

void random_permutation(int arr[]) {
    int i;
    for (i = 0; i < N; i++)
        arr[i] = i;
    for (i = 0; i < N; i++) {
        int j = rand() % N;
        int t = arr[i];
        arr[i] = arr[j];
        arr[j] = t;
    }
}

double decodePenalty_and_record(int chrom[], int out_ev[], int out_port[], int out_start[], int out_end[]) {
    int i, j;
    int port_free_time[MAX_PORTS] = {0};
    double total_wait = 0;
    double urgent_penalty = 0;

    for (i = 0; i < N; i++) {
        int ev = chrom[i];
        int chosen_port = 0;
        int earliest_free = port_free_time[0];
        for (j = 1; j < P; j++) {
            if (port_free_time[j] < earliest_free) {
                earliest_free = port_free_time[j];
                chosen_port = j;
            }
        }
        int start = max(arrival[ev], earliest_free);
        int end = start + duration_min[ev];
        int wait = start - arrival[ev];
        port_free_time[chosen_port] = end;
        total_wait += wait;
        if (urgency_flag[ev]) urgent_penalty += wait * 6;

        if (out_ev != NULL) {
            out_ev[i] = ev;
            out_port[i] = chosen_port;
            out_start[i] = start;
            out_end[i] = end;
        }
    }

    int max_end = 0;
    for (i = 0; i < P; i++)
        if (port_free_time[i] > max_end) max_end = port_free_time[i];

    double penalty = total_wait + urgent_penalty + 0.01 * max_end;
    return penalty;
}

void initPopulation() {
    for (int i = 0; i < POP_SIZE; i++) random_permutation(population[i]);
}

void evaluatePopulation() {
    for (int i = 0; i < POP_SIZE; i++) {
        double penalty = decodePenalty_and_record(population[i], NULL, NULL, NULL, NULL);
        fitness_arr[i] = 1.0 / (1.0 + penalty);
    }
}

int tournament_select(int k = 3) {
    int best = rand() % POP_SIZE;
    for (int i = 1; i < k; i++) {
        int r = rand() % POP_SIZE;
        if (fitness_arr[r] > fitness_arr[best]) best = r;
    }
    return best;
}

void order_crossover(int parentA[], int parentB[], int childA[], int childB[]) {
    int l = rand() % N;
    int r = rand() % N;
    if (l > r) swap(l, r);
    int usedA[MAX_EV] = {0};
    int usedB[MAX_EV] = {0};
    int i, j, posA = 0, posB = 0;

    for (i = l; i <= r; i++) {
        childA[i] = parentA[i];
        usedA[parentA[i]] = 1;
        childB[i] = parentB[i];
        usedB[parentB[i]] = 1;
    }

    for (i = 0; i < N; i++) {
        int idx = (r + 1 + i) % N;
        while (usedA[parentB[posA]]) posA++;
        childA[idx] = parentB[posA++];
        while (usedB[parentA[posB]]) posB++;
        childB[idx] = parentA[posB++];
    }
}

void swap_mutation(int chrom[], double mut_prob) {
    if ((rand() / (double)RAND_MAX) < mut_prob) {
        int i = rand() % N;
        int j = rand() % N;
        int t = chrom[i];
        chrom[i] = chrom[j];
        chrom[j] = t;
    }
}

void runGA_and_print_best() {
    evaluatePopulation();
    double best_fitness = 0;
    int best_index = 0;

    for (int gen = 0; gen < MAX_GEN; gen++) {
        int newpop[POP_SIZE][MAX_EV];

        // Elitism
        for (int k = 0; k < N; k++) newpop[0][k] = population[best_index][k];

        for (int i = 1; i < POP_SIZE; i += 2) {
            int p1 = tournament_select();
            int p2 = tournament_select();
            int childA[MAX_EV], childB[MAX_EV];
            order_crossover(population[p1], population[p2], childA, childB);
            swap_mutation(childA, 0.15);
            swap_mutation(childB, 0.15);
            for (int k = 0; k < N; k++) {
                newpop[i][k] = childA[k];
                if (i + 1 < POP_SIZE) newpop[i + 1][k] = childB[k];
            }
        }

        for (int i = 0; i < POP_SIZE; i++)
            for (int k = 0; k < N; k++)
                population[i][k] = newpop[i][k];

        evaluatePopulation();

        // Update best
        for (int i = 0; i < POP_SIZE; i++) {
            if (fitness_arr[i] > best_fitness) {
                best_fitness = fitness_arr[i];
                best_index = i;
            }
        }

        if (gen % 50 == 0)
            cout << "[Gen " << gen << "] best fitness = " << best_fitness << endl;
    }

    // Decode best
    int ev_order[MAX_EV], port_used[MAX_EV], start[MAX_EV], end[MAX_EV];
    double final_penalty = decodePenalty_and_record(population[best_index], ev_order, port_used, start, end);

    cout << "\n=== BEST SCHEDULE (penalty = " << final_penalty << ") ===\n";
    cout << "Pos\tEV\tPort\tStart\tEnd\tWait\tUrgent\n";
    double total_wait = 0;
    for (int i = 0; i < N; i++) {
        int ev = ev_order[i];
        int wait = start[i] - arrival[ev];
        total_wait += wait;
        cout << i << "\t" << ev << "\t" << port_used[i] << "\t" << start[i]
             << "\t" << end[i] << "\t" << wait << "\t" << urgency_flag[ev] << "\n";
    }
    cout << "Total waiting time = " << total_wait << " minutes\nOrder: ";
    for (int i = 0; i < N; i++) cout << ev_order[i] << " ";
    cout << endl;
}

void read_input_interactive() {
    cout << "EV Charging scheduling using GA (array-based).\n";
    cout << "Enter number of EVs: ";
    cin >> N;
    cout << "Enter number of ports: ";
    cin >> P;
    cout << "Enter EV data (arrival duration urgency):\n";
    for (int i = 0; i < N; i++) {
        cin >> arrival[i] >> duration_min[i] >> urgency_flag[i];
        ev_id_map[i] = i;
    }
}

int main() {
    srand(time(0));
    read_input_interactive();
    initPopulation();
    runGA_and_print_best();
    return 0;
}
