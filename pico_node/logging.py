class AbstractLogger:
    def __init__(self):
        pass

    def log(self, msg):
        pass

    def log_error(self, msg):
        pass

    def log_warning(self, msg):
        pass

    def log_info(self, msg):
        pass

    def log_debug(self, msg):
        pass


class Logger(AbstractLogger):
    # log_format = "[{level}] {msg}"
    # log_level = "DEBUG"

    def __init__(self):
        super().__init__()

    def _log(self, level, msg):
        print(f'{level}:\t{msg:<30} ')

    def log(self, msg):
        self._log("LOG", msg)

    def log_error(self, msg):
        self._log("\033[1;31mERROR\033[0m", msg)

    def log_warning(self, msg):
        self._log("\033[1;33mWARNING\033[0m", msg)

    def log_info(self, msg):
        self._log("\033[1;32mINFO\033[0m", msg)

    def log_debug(self, msg):
        self._log("DEBUG", msg)
