#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string.h>

#ifdef TEST_SH
# define TEST 1
#else
# define TEST 0
#endif

int print_error(char *str)
{
	int i = 0;

	while (str[i])
		write(STDERR_FILENO, &str[i++], 1);
	return (1);
}

int fatal(char **ptr)
{
	free(ptr);
	exit(print_error("error: fatal error\n"));
}

int tab_len(char **cmd)
{
	int i = 0;

	if (!cmd)
		return (0);
	while (cmd[i])
		i++;
	return (i);
}

int size_cmd_char(char **cmd, char *str)
{
	int i = 0;

	if (!cmd)
		return (0);
	while (cmd[i])
	{
		if (!strcmp(cmd[i], str))
			return (i);
		i++;
	}
	return (i);
}

char **find_next_pipe(char **cmd)
{
	int i = 0;

	if (!cmd)
		return (NULL);
	while (cmd[i])
	{
		if (!strcmp(cmd[i], "|"))
			return (&cmd[i + 1]);
		i++;
	}
	return (NULL);
}

char **get_cmd(char **av, int *i)
{
	int size = size_cmd_char(&av[*i], ";");
	int j = 0;
	char **cmd;

	if (!size)
		return (NULL);
	if (!(cmd = (char **)malloc(sizeof(char *) * (size + 1))))
		fatal(NULL);
	while (j < size)
	{
		cmd[j] = av[*i + j];
		j++;
	}
	cmd[j] = NULL;
	*i += size;
	return (cmd);
}

int builtin_cd(char **cmd)
{
	if (tab_len(cmd) != 2)
		return (print_error("error: cd: bad arguments\n"));
	if (chdir(cmd[1]) < 0)
	{
		print_error("error: cd: cannot change directory ");
		print_error(cmd[1]);
		print_error("\n");
	}
	return (0);
}

int exec_cmd(char **cmd, char **env, char **ptr)
{
	pid_t pid;

	if ((pid = fork()) < 0)
		fatal(ptr);
	if (pid == 0)
	{
		if (execve(cmd[0], cmd, env) < 0)
		{
			print_error("error: cannot execute ");
			print_error(cmd[0]);
			free(ptr);
			exit(print_error("\n"));
		}
	}
	waitpid(0, NULL, 0);
	return (0);
}

int exec_son(char **cmd, char **env, char **cmd_tmp, int in, int fd_pipe[2])
{
	if (dup2(in, STDIN_FILENO) < 0)
		fatal(cmd);
	if (find_next_pipe(cmd_tmp) && dup2(fd_pipe[1], STDOUT_FILENO) < 0)
		fatal(cmd);
	close(in);
	close(fd_pipe[0]);
	close(fd_pipe[1]);
	cmd_tmp[size_cmd_char(cmd_tmp, "|")] = NULL;
	exec_cmd(cmd_tmp, env, cmd);
	free(cmd);
	exit(0);
}

int execute(char **cmd, char **env)
{
	int in;
	int fd_pipe[2];
	int nb_wait = 0;
	char **cmd_tmp = cmd;
	pid_t pid;

	if (!find_next_pipe(cmd))
		return (exec_cmd(cmd, env, cmd));
	if ((in = dup(STDIN_FILENO)) < 0)
		fatal(cmd);
	while (cmd_tmp)
	{
		if (pipe(fd_pipe) < 0 || (pid = fork()) < 0)
			fatal(cmd);
		if (pid == 0)
			exec_son(cmd, env, cmd_tmp, in, fd_pipe);
		else
		{
			if (dup2(fd_pipe[0], in) < 0)
				fatal(cmd);
			close(fd_pipe[0]);
			close(fd_pipe[1]);
			cmd_tmp = find_next_pipe(cmd_tmp);
			nb_wait++;
		}
	}
	close(in);
	while (nb_wait-- >= 0)
		waitpid(0, NULL, 0);
	return (0);
}

int main(int ac, char **av, char **env)
{
	char **cmd = NULL;
	int i = 1;

	while (i < ac)
	{
		cmd = get_cmd(av, &i);
		if (cmd && !strcmp(cmd[0], "cd"))
			builtin_cd(cmd);
		else if (cmd)
			execute(cmd, env);
		free(cmd);
		cmd = NULL;
		i++;
	}
	if (TEST)
		while (1);
	return (0);
}
