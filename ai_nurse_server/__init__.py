# Initialize PyMySQL as MySQLdb for cPanel Shared Hosting compatibility
try:
    import pymysql
    pymysql.install_as_MySQLdb()
except ImportError:
    pass
