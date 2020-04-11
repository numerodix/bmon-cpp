#include <chrono>
#include <csignal>
#include <iostream>
#include <map>
#include <memory>
#include <thread>

#include "sampling/ip_cmd_sampler.hpp"
#include "sampling/procfs_sampler.hpp"
#include "sampling/sysfs_sampler.hpp"
#include "termui/bar_chart.hpp"
#include "termui/signals.hpp"
#include "termui/terminal_driver.hpp"
#include "termui/terminal_mode.hpp"
#include "termui/terminal_surface.hpp"
#include "termui/terminal_window.hpp"

namespace bmon {
namespace termui {

void display_bar_chart(const std::unique_ptr<sampling::Sampler> &sampler,
                       const std::string &iface_name,
                       BarChart &bar_chart) {
    std::vector<uint64_t> rxs{};

    sampling::Sample prev_sample = sampler->get_sample(iface_name);

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        sampling::Sample sample = sampler->get_sample(iface_name);

        auto rx = sample.rx - prev_sample.rx;
        rxs.push_back(rx);

        // make sure the vector isn't longer than the width of the display
        if (rxs.size() > bar_chart.get_width()) {
            rxs.erase(rxs.begin());
        }

        prev_sample = sample;

        bar_chart.draw_bars_from_right(rxs);
    }
}

void run(const std::string &iface_name) {
    std::unique_ptr<sampling::Sampler> sys_sampler{
        new sampling::SysFsSampler()};

    SignalSuspender susp_sigint{SIGINT};
    SignalSuspender susp_sigwinch{SIGWINCH};

    TerminalModeSet mode_set{};
    auto mode_setter =
        mode_set.local_off(ECHO).local_off(ICANON).build_setter(&susp_sigint);
    TerminalModeGuard mode_guard{&mode_setter};

    TerminalDriver driver{stdin, stdout};
    auto terminal_window =
        TerminalWindow::create(&driver, &susp_sigwinch);
    // unique_ptr to ensure deletion of terminal_window
    auto win = std::unique_ptr<TerminalWindow>(terminal_window);

    TerminalSurface surface{terminal_window, 10};
    BarChart bar_chart{&surface};

    display_bar_chart(sys_sampler, iface_name, bar_chart);
}

} // namespace termui
} // namespace bmon

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cout << "Must pass <iface_name>\n";
        exit(EXIT_FAILURE);
    }

    std::string iface_name{argv[1]};

    // We expect to get a Ctrl+C. Install a SIGINT handler that throws an
    // exception such that we can unwind orderly and enter the catch block
    // below.
    signal(SIGINT, bmon::termui::sigint_handler);

    // We desperately need to wrap the execution in a try/catch otherwise an
    // uncaught exception will terminate the program bypassing all destructors
    // and leave the terminal in a corrupted state.
    try {
        bmon::termui::run(iface_name);
    } catch (bmon::termui::InterruptException &e) {
        // This is the expected way to stop the program. Nothing to do here.
    } catch (std::exception &e) {
        std::cerr << "Trapped uncaught exception:\n  " << e.what();
        exit(EXIT_FAILURE);
    } catch (...) {
        std::cerr << "This is the last resort exception handler. I have no "
                     "state about the error.";
        exit(EXIT_FAILURE);
    }

    return 0;
}