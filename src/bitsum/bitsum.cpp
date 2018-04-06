#include "common/CommandLine.hpp"
#include "common/ConsoleTools.hpp"
#include "Core/Node.hpp"
#include "Core/WalletNode.hpp"
#include "Core/Config.hpp"
#include "platform/ExclusiveLock.hpp"
#include "logging/LoggerManager.hpp"
#include "platform/Network.hpp"
#include "version.hpp"
#include "common/Base58.hpp"
#include "seria/BinaryInputStream.hpp"
#include "seria/BinaryOutputStream.hpp"

using namespace bytecoin;
using namespace common;

#if defined(_WIN32)
extern "C" _declspec(dllexport) int __DaemonRun()
#else
extern "C" int __DaemonRun()
#endif
{
	try
	{
		common::console::UnicodeConsoleSetup console_setup;
		auto idea_start = std::chrono::high_resolution_clock::now();

		common::CommandLine cmd(1, 0);
		bytecoin::Config config(cmd);
		bytecoin::Currency currency(config.is_testnet);
		const std::string coinFolder = config.get_data_folder();

		platform::ExclusiveLock coin_lock(coinFolder, "bitsumd.lock");

		logging::LoggerManager logManager;
		logManager.configure_default(config.get_data_folder("logs"), "bitsumd-");
		
		BlockChainState block_chain(logManager, config, currency);
		
		boost::asio::io_service io;
		platform::EventLoop run_loop(io);
		
		Node node(logManager, config, block_chain);
		
		auto idea_ms =
			std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - idea_start);
		std::cout << "bitsumd started seconds=" << double(idea_ms.count()) / 1000 << std::endl;
		while (!io.stopped()) {
			if (node.on_idle())  // Using it to load blockchain
				io.poll();
			else
				io.run_one();
		}

		return 0;
	}
	catch (const std::exception&)
	{
		return 1;
	}
}

#if defined(_WIN32)
extern "C" _declspec(dllexport) int __WalletRun(char path[], char password[])
#else
extern "C" int __WalletRun(char path[], char password[])
#endif
{
	common::console::UnicodeConsoleSetup console_setup;
	auto idea_start = std::chrono::high_resolution_clock::now();
	common::CommandLine cmd(1, 0);
	bytecoin::Config config(cmd);
	bytecoin::Currency currency(config.is_testnet);

	const std::string coinFolder = config.get_data_folder();
	std::unique_ptr<platform::ExclusiveLock> walletcache_lock;
	std::unique_ptr<Wallet> wallet;

	try {
		wallet = std::make_unique<Wallet>(path, password, false);
		walletcache_lock = std::make_unique<platform::ExclusiveLock>(
			config.get_data_folder("wallet_cache"), wallet->get_cache_name() + ".lock");
	}
	catch (const std::ios_base::failure &ex) {
		std::cout << ex.what() << std::endl;
		return api::WALLET_FILE_READ_ERROR;
	}
	catch (const platform::ExclusiveLock::FailedToLock &ex) {
		std::cout << ex.what() << std::endl;
		return api::WALLET_WITH_THE_SAME_VIEWKEY_IN_USE;
	}
	catch (const Wallet::Exception &ex) {
		std::cout << ex.what() << std::endl;
		return ex.return_code;
	}

	logging::LoggerManager logManagerWalletNode;
	logManagerWalletNode.configure_default(config.get_data_folder("logs"), "wallet-");

	WalletState wallet_state(*wallet, logManagerWalletNode, config, currency);
	boost::asio::io_service io;
	platform::EventLoop run_loop(io);

	std::unique_ptr<Node> node;
	std::unique_ptr<WalletNode> wallet_node;
	try {
		wallet_node = std::make_unique<WalletNode>(nullptr, logManagerWalletNode, config, wallet_state);
	}
	catch (const boost::system::system_error &ex) {
		std::cout << ex.what() << std::endl;
		return api::WALLETD_BIND_PORT_IN_USE;
	}

	auto idea_ms =
		std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - idea_start);
	std::cout << "wallet-rpc started seconds=" << double(idea_ms.count()) / 1000 << std::endl;

	while (!io.stopped()) {
		if (node && node->on_idle())  // We load blockchain there
			io.poll();
		else
			io.run_one();
	}

	return 0;
}

#if defined(_WIN32)
extern "C" _declspec(dllexport) int __WalletCreateContainer(char path[], char password[])
#else
extern "C" int __WalletCreateContainer(char path[], char password[])
#endif
{
	std::unique_ptr<Wallet> wallet;

	try {
		wallet = std::make_unique<Wallet>(path, password, true);
		
		//Currency::parseAccountAddressString getAccountAddressAsStr(public_address_base58_prefix, account_public_address);
		return 0;
	}
	catch (const std::ios_base::failure & ex) {
		std::cout << ex.what() << std::endl;
		return api::WALLET_FILE_READ_ERROR;
	}
	catch (const platform::ExclusiveLock::FailedToLock & ex) {
		std::cout << ex.what() << std::endl;
		return api::WALLET_WITH_THE_SAME_VIEWKEY_IN_USE;
	}
	catch (const Wallet::Exception & ex) {
		std::cout << ex.what() << std::endl;
		return ex.return_code;
	}
}

#if defined(_WIN32)
extern "C" _declspec(dllexport) int __WalletImportContainer(char path[], char password[], char keys[])
#else
extern "C" int __WalletImportContainer(char path[], char password[], char keys[])
#endif
{
	std::unique_ptr<Wallet> wallet;

	try {
		wallet = std::make_unique<Wallet>(path, password, true, keys);

		return 0;
	}
	catch (const std::ios_base::failure & ex) {
		std::cout << ex.what() << std::endl;
		return api::WALLET_FILE_READ_ERROR;
	}
	catch (const platform::ExclusiveLock::FailedToLock & ex) {
		std::cout << ex.what() << std::endl;
		return api::WALLET_WITH_THE_SAME_VIEWKEY_IN_USE;
	}
	catch (const Wallet::Exception & ex) {
		std::cout << ex.what() << std::endl;
		return ex.return_code;
	}
}

#if defined(_WIN32)
extern "C" _declspec(dllexport) int __WalletChangeContainerPassword(char path[], char oldPassword[], char newPassword[])
#else
extern "C" int __WalletChangeContainerPassword(char path[], char oldPassword[], char newPassword[])
#endif
{
	std::unique_ptr<Wallet> wallet;

	try {
		wallet = std::make_unique<Wallet>(path, oldPassword, false);
		wallet->set_password(newPassword);
		return 0;
	}
	catch (const std::ios_base::failure & ex) {
		std::cout << ex.what() << std::endl;
		return api::WALLET_FILE_READ_ERROR;
	}
	catch (const platform::ExclusiveLock::FailedToLock & ex) {
		std::cout << ex.what() << std::endl;
		return api::WALLET_WITH_THE_SAME_VIEWKEY_IN_USE;
	}
	catch (const Wallet::Exception & ex) {
		std::cout << ex.what() << std::endl;
		return ex.return_code;
	}
}

#if defined(_WIN32)
extern "C" _declspec(dllexport) bool __CheckAddress(char address[])
#else
extern "C" bool __CheckAddress(char address[])
#endif
{
	BinaryArray data;
	uint64_t prefix = 154;
	AccountPublicAddress adr;

	if (!common::base58::decode_addr(address, prefix, data))
		return false;
	try {
		seria::from_binary(adr, data);
	}
	catch (const std::exception &) {
		return false;
	}
	return key_isvalid(adr.spend_public_key) && key_isvalid(adr.view_public_key);
}