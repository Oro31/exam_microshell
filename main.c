#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#define TYPE_END	0
#define TYPE_PIPE	1
#define TYPE_BREAK	2

typedef struct	s_cmd
{
	char			**argv;
	int				length;
	int				type;
	int				pipes[2];
	struct s_cmd	*prev;
	struct s_cmd	*next;
}				t_cmd;

int	ft_strlen(char *str)
{
	int	i;

	if (!str)
		return (0);
	i = 0;
	while (str[i])
		i++;
	return (i);
}

int	err_msg(char *str)
{
	if (str)
		write(2, str, ft_strlen(str));
	return (EXIT_FAILURE);
}

int	err_fatal_exit(void)
{
	err_msg("error: fatal\n");
	exit(EXIT_FAILURE);
	return (EXIT_FAILURE);
}

char	*ft_strdup(char *str)
{
	char	*copy;
	int		i;

	copy = malloc(sizeof(char) * (ft_strlen(str) + 1));
	if (!copy)
	{
		err_fatal_exit();
		return (NULL);
	}
	i = -1;
	while (str[++i])
		copy[i] = str[i];
	copy[i] = '\0';
	return (copy);
}

int	new_arg(t_cmd *cmd, char *arg)
{
	char	**tmp;
	int		i;

	tmp = malloc(sizeof(char *) * (cmd->length + 2));
	if (!tmp)
		return (err_fatal_exit());
	i = -1;
	while (++i < cmd->length)
		tmp[i] = cmd->argv[i];
	if (cmd->length > 0)
		free(cmd->argv);
	cmd->argv = tmp;
	cmd->argv[i] = ft_strdup(arg);
	cmd->argv[++i] = NULL;
	cmd->length++;
	return (EXIT_SUCCESS);
}

int	new_cmd(t_cmd **cmd, char *arg)
{
	t_cmd	*new;

	new = NULL;
	new = malloc(sizeof(t_cmd));
	if (!new)
		return (err_fatal_exit());
	new->argv = NULL;
	new->length = 0;
	new->type = TYPE_END;
	new->prev = NULL;
	new->next = NULL;
	if (*cmd)
	{
		(*cmd)->next = new;
		new->prev = *cmd;
	}
	*cmd = new;
	return (new_arg(new, arg));
}

int	get_head_of_list(t_cmd **cmd)
{
	while ((*cmd) && (*cmd)->prev)
		*cmd = (*cmd)->prev;
	return (EXIT_SUCCESS);
}

int	free_list(t_cmd **cmd)
{
	t_cmd	*tmp;
	int		i;

	get_head_of_list(cmd);
	while (*cmd)
	{
		tmp = (*cmd)->next;
		i = -1;
		while (++i < (*cmd)->length)
			free((*cmd)->argv[i]);
		free((*cmd)->argv);
		free(*cmd);
		*cmd = tmp;
	}
	*cmd = NULL;
	return (EXIT_SUCCESS);
}

int	parse_arg(t_cmd **cmd, char *arg)
{
	int	is_break;

	is_break = 0;
	if (strcmp(";", arg) == 0)
		is_break = 1;
	if (is_break && !*cmd)
		return (EXIT_SUCCESS);
	else if (!is_break && (!*cmd || (*cmd)->type > TYPE_END))
		return (new_cmd(cmd, arg));
	else if (strcmp("|", arg) == 0)
		(*cmd)->type = TYPE_PIPE;
	else if (is_break)
		(*cmd)->type = TYPE_BREAK;
	else
		return (new_arg(*cmd, arg));
	return (EXIT_SUCCESS);
}

int	ft_execve(t_cmd *cmd, char **env)
{
	pid_t	pid;
	t_cmd	*tmp;
	int		ret;
	int		status;
	int		pipe_open;

	tmp = NULL;
	ret = EXIT_FAILURE;
	pipe_open = 0;
	if (cmd->type == TYPE_PIPE || (cmd->prev && cmd->prev->type == TYPE_PIPE))
	{
		pipe_open = 1;
		if (pipe(cmd->pipes))
			return (err_fatal_exit());
	}
	pid = fork();
	if (pid < 0)
		return (err_fatal_exit());
	else if (pid == 0)
	{
		if (cmd->type == TYPE_PIPE)
		{
			if (dup2(cmd->pipes[1], STDOUT_FILENO) < 0)
				return (err_fatal_exit());
		}
		if (cmd->prev && cmd->prev->type == TYPE_PIPE)
		{
			if (dup2(cmd->prev->pipes[0], STDIN_FILENO) < 0)
				return (err_fatal_exit());
		}
		ret = execve(cmd->argv[0], cmd->argv, env);
		if (ret == -1)
		{
			err_msg("error: cannot execute ");
			err_msg(cmd->argv[0]);
			err_msg("\n");
		}
		exit(ret);
	}
	else
	{
		waitpid(pid, &status, 0);
		if (pipe_open)
		{
			close(cmd->pipes[1]);
			if (!cmd->next || cmd->type == TYPE_BREAK)
				close(cmd->pipes[0]);
		}
		if (cmd->prev && cmd->prev->type == TYPE_PIPE)
			close(cmd->prev->pipes[0]);
		if (WIFEXITED(status))
			ret = WEXITSTATUS(status);
	}
	return (ret);
}

int	exec_cmd(t_cmd **cmd, char **env)
{
	t_cmd	*tmp;
	int		ret;

	ret = EXIT_SUCCESS;
	get_head_of_list(cmd);
	while (*cmd)
	{
		tmp = *cmd;
		if (strcmp("cd", tmp->argv[0]) == 0)
		{
			ret = EXIT_SUCCESS;
			if (tmp->length < 2)
				ret = err_msg("error: cd: bad arguments\n");
			else if (chdir(tmp->argv[1]))
			{
				ret = err_msg("error: cd: cannot change directory to ");
				err_msg(tmp->argv[1]);
				err_msg("\n");
			}
		}
		else
			ret = ft_execve(tmp, env);
		if (!(*cmd)->next)
			break ;
		*cmd = (*cmd)->next;
	}
	return (ret);
}

int	main(int argc, char **argv, char **env)
{
	t_cmd	*cmd;
	int		i;
	int		ret;

	ret = EXIT_SUCCESS;
	cmd = NULL;
	tmp = NULL;
	i = 0;
	while (++i < argc)
		parse_arg(&cmd, argv[i]);
	if (cmd)
		ret = exec_cmd(&cmd, env);
	free_list(&cmd);
	return (ret);
}
