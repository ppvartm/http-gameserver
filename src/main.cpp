//#include "sdk.h"
#include <iostream>
#include <thread>
#include <memory>
#include <cstdlib>
#include <stdexcept>


#include <boost/program_options.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/io_context.hpp>

#include "json_tools/json_loader.h"
#include "web/request_handler.h"
#include "web/http_server.h"
#include "web/log.h"
#include "web/timer.h"
#include "extra/extra_data.h"
#include "database_tools/postgres.h"

using namespace std::literals;
namespace net = boost::asio;
namespace sys = boost::system;
namespace po = boost::program_options;

namespace {

 // starting fn func on n threads
    template <typename Fn>
    void RunWorkers(unsigned num_of_threads, const Fn& fn) {
        num_of_threads = std::max(1u, num_of_threads);
        std::vector<std::jthread> workers;
        workers.reserve(num_of_threads - 1);       
        while (--num_of_threads) {
            workers.emplace_back(fn);
        }
        fn();
    }

    struct Args {
        std::string tick_period;
        std::string config_file_path;
        std::string static_dir_path;
        std::string state_file_path;
        std::string log_file_path;
        std::string save_state_period;
        std::string random_spawn;
    };

    std::optional<Args> ParseCommandLine(int argc, const char* const argv[]) {
        po::options_description desc{ "All options"s };
        Args args;
        desc.add_options()
            ("help,h", "Show help")
            ("tick-period,t", po::value(&args.tick_period)->value_name("millisecondse"s), "Set tick period")
            ("config-file,c", po::value(&args.config_file_path)->value_name("file"s), "Set config file path")
            ("www-root,w", po::value(&args.static_dir_path)->value_name("dir"s), "Set static files root")
            ("state-file", po::value(&args.state_file_path)->value_name("state"s), "Set state file path")
            ("log-file", po::value(&args.log_file_path)->value_name("log"s), "Set log file path")
            ("save-state-period", po::value(&args.save_state_period)->value_name("state-period"s),"Set period of auto save state")
            ("randomize-spawn-points", "Set random-spawn configuration");

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.contains("help"s)) {
            std::cout << desc;
            return std::nullopt;
        }

        if (!vm.contains("config-file"s)) {
            throw std::runtime_error("Config file path is not specified"s);
        }
        if (!vm.contains("www-root"s)) {
            throw std::runtime_error("Static dir path is not specified"s);
        }
        if (!vm.contains("log-file"s)) {
            throw std::runtime_error("Log file path is not specified"s);
        }
        if (vm.contains("randomize-spawn-points"s)) {
             args.random_spawn = "random";
        }
        if (!vm.contains("state-file"s)) {
            args.state_file_path = "not-state-save";
        }
        if (!vm.contains("save-state-period"s)) {
            args.save_state_period = "without-state-period";
        }
        return args;
    }

    std::optional <std::pair<bool, std::filesystem::path>> IsSave(const std::optional<Args>& args) {
        if (args->state_file_path != "not-state-save") {
            auto state_file_path = std::filesystem::weakly_canonical(args->state_file_path);
            return {{true, state_file_path }};
        }
        return std::nullopt;
    }
    std::optional <std::pair<bool, int>> IsAutoSave(const std::optional<Args>& args) {
        if (args->save_state_period != "without-state-period")
            return { {true,std::stoi(args->save_state_period)} };
        return std::nullopt;
    }
    
    void SetSaveParams(const std::optional<Args>& args, std::filesystem::path& state_file_path, bool& is_save, bool& is_auto_save, int& save_interval) {
        if (auto save_params = IsSave(args)) {
            is_save = save_params.value().first;
            state_file_path = save_params.value().second;

            if (auto auto_save_params = IsAutoSave(args)) {
                is_auto_save = auto_save_params.value().first;
                save_interval = auto_save_params.value().second;
            }
        }
    }


    constexpr const char GAME_DB_URL[]{ "GAME_DB_URL" };

    std::string GetDatabaseUrl() {
        std::string db_url;
        if (const auto* url = std::getenv(GAME_DB_URL)) {
            db_url = url;
        }
        else {
            throw std::runtime_error(GAME_DB_URL + " environment variable not found"s);
        }
        return db_url;
    }

}  // namespace



int main(int argc, const char* argv[]) {
    try {
        //getting command line args
        auto args = ParseCommandLine(argc, argv);

        logging::add_common_attributes();
        logging::add_file_log(
            logging::keywords::file_name = args->log_file_path,
            logging::keywords::format = MyFormatter,
            logging::keywords::auto_flush = true,
            logging::keywords::rotation_size = 10 * 1024 * 1024
        );

        if (!args) {
            throw std::runtime_error("Please, specify the requied parametrs");
        }

        auto config_file_path = std::filesystem::weakly_canonical( args->config_file_path );
        auto static_dir_path = std::filesystem::weakly_canonical ( args->static_dir_path );
        
        if (!std::filesystem::exists(config_file_path))
            throw std::runtime_error("Config file doesn't exist");
        if (!std::filesystem::exists(static_dir_path))
            throw std::runtime_error("Static files dir doesn't exist");

        std::filesystem::path state_file_path;
        int save_interval = 0;
        bool is_save = false, is_auto_save = false;


        SetSaveParams(args, state_file_path, is_save, is_auto_save, save_interval);
       

        postgres_tools::PostgresDatabase database(GetDatabaseUrl());

        // 1. maps from file and setting random player position
        model::Game game = json_loader::LoadGame(config_file_path);
        if (args->random_spawn == "random") {
            game.SetRandomSpawn();
        }

        extra_data::Json_data lost_objects_json_data = json_loader::LoadExtraData(config_file_path);

        // 2. io_context & strand
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);
        net::strand strand = net::make_strand(ioc);

 
        // 3. http_handler + logging decorator
        http_handler::RequestHandler handler {game, lost_objects_json_data, strand, database };
        handler.SetFilePath(static_dir_path);
        handler.SetSerializationParams(is_save, is_auto_save, save_interval, state_file_path);
        if (std::filesystem::exists(state_file_path)) {
            handler.Deserialize();
        }
        http_handler::LoggingRequestHandler logging_handler(handler);
      

        // 4.SIGINT & SIGTERM
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc, &handler, is_save](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
           if (!ec) {
                ioc.stop();
                if (is_save)
                   handler.Serialize();
            }
        });
        // 5. starting http_handler
        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;
        http_server::ServeHttp(ioc, { address, port }, [&logging_handler](auto&& req, auto&& send) {
            logging_handler(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
        });
        
        // 6. timer autoupdate
        std::shared_ptr<Timer::Ticker> ticker;
        if (args->tick_period != "") {
            handler.SetAutomaticTick();
              ticker = std::make_shared<Timer::Ticker>(strand, std::chrono::milliseconds(std::stoi(args->tick_period)),
                 [&handler](std::chrono::milliseconds delta) {
                     handler.Tick(delta); 
                 });
              ticker->Start();
        }


      //   std::cout << "Server has started..."sv << std::endl;
        ServerStartLog(port, address);

        // 7. handling async operation
        RunWorkers(std::max(1u, num_threads), [&ioc] {
            ioc.run();
        });

            handler.Serialize();

        ServerStopLog(0);
    } catch (const std::exception& ex) {

        ServerStopLog(EXIT_FAILURE, ex.what());
        return EXIT_FAILURE;
    }

}
